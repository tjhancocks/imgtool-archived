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

#ifndef SHELL
#define SHELL

#include <stdint.h>

#include "vfs.h"
#include "shell_scripting.h"

struct shell {
    // File System
    vfs_t filesystem;

    // Commands
    shell_command_t first_command;
    
    // User Prompts
    uint32_t buffer_size;
    
    // Import Buffer
    uint32_t import_buffer_size;
    uint8_t *import_buffer;
    
    // Flags
    uint8_t running:1;
    uint8_t reserved:7;
};
typedef struct shell * shell_t;

shell_t shell_init(vfs_t vfs);
void shell_do(shell_t shell);

void shell_add_command(shell_t shell, shell_command_t command);

#endif
