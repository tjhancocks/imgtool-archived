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

#include <stdlib.h>
#include <assert.h>
#include <string.h>

#include "shell.h"
#include "shell_scripting.h"

shell_command_t shell_command_create(const char *name, shell_command_imp_t imp)
{
    shell_command_t cmd = calloc(1, sizeof(*cmd));
    cmd->name = name;
    cmd->impl = imp;
    return cmd;
}

void shell_command_destroy(shell_command_t cmd)
{
    if (cmd) {
        shell_command_destroy(cmd->next);
    }
    free(cmd);
}


shell_command_t shell_command_for(shell_t shell, const char *name)
{
    assert(shell);
    shell_command_t cmd = shell->first_command;
    while (cmd) {
        if (strcmp(cmd->name, name) == 0) {
            return cmd;
        }
        cmd = cmd->next;
    }
    return NULL;
}

void shell_execute(shell_t shell, int argc, const char *argv[])
{
    assert(shell);
    assert(argc > 0);
    assert(argv);

    shell_command_t cmd = shell_command_for(shell, argv[0]);
    if (!cmd) {
        fprintf(stderr, "Unrecognised command: %s\n", argv[0]);
        return;
    }

    cmd->impl(shell, argc, argv);
}
