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

#include <shell/commands.h>
#include <shell/shell.h>

#include <shell/exit.h>
#include <shell/echo.h>
#include <shell/format.h>
#include <shell/mount.h>
#include <shell/touch.h>
#include <shell/import.h>
#include <shell/write.h>
#include <shell/mkdir.h>
#include <shell/ls.h>
#include <shell/set.h>
#include <shell/attach.h>
#include <shell/init.h>
#include <shell/rm.h>
#include <shell/read.h>
#include <shell/export.h>
#include <shell/grub.h>

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
    shell_add_command(shell, shell_command_create("export", shell_export));
    shell_add_command(shell, shell_command_create("write", shell_write));
    shell_add_command(shell, shell_command_create("read", shell_read));
    shell_add_command(shell, shell_command_create("mkdir", shell_mkdir));
    shell_add_command(shell, shell_command_create("ls", shell_ls));
    shell_add_command(shell, shell_command_create("set", shell_set));
    shell_add_command(shell, shell_command_create("setu", shell_setu));
    shell_add_command(shell, shell_command_create("attach", shell_attach));
    shell_add_command(shell, shell_command_create("detach", shell_detach));
    shell_add_command(shell, shell_command_create("init", shell_init_dev));
    shell_add_command(shell, shell_command_create("rm", shell_rm));
    shell_add_command(shell, shell_command_create("grub", shell_grub));
}

