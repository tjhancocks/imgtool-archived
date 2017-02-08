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

#include <assert.h>

#include "shell_commands.h"
#include "shell.h"

#include "shell_exit.h"
#include "shell_echo.h"
#include "shell_format.h"
#include "shell_mount.h"
#include "shell_touch.h"
#include "shell_import.h"
#include "shell_write.h"
#include "shell_mkdir.h"

void shell_register_commands(shell_t shell)
{
    assert(shell);
    shell_add_command(shell, shell_command_create("exit", shell_exit));
    shell_add_command(shell, shell_command_create("echo", shell_echo));
    shell_add_command(shell, shell_command_create("format", shell_format));
    shell_add_command(shell, shell_command_create("mount", shell_mount));
    shell_add_command(shell, shell_command_create("unmount", shell_unmount));
    shell_add_command(shell, shell_command_create("touch", shell_touch));
    shell_add_command(shell, shell_command_create("import", shell_import));
    shell_add_command(shell, shell_command_create("write", shell_write));
    shell_add_command(shell, shell_command_create("mkdir", shell_mkdir));
}
