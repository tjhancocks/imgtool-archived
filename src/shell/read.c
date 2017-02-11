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
#include <stdlib.h>

#include <shell/read.h>
#include <shell/shell.h>
#include <vfs/vfs.h>

void shell_read(struct shell *shell, int argc, const char *argv[])
{
    assert(shell);
    
    if (argc != 2) {
        fprintf(stderr, "Expected a single argument for the file name.\n");
        return;
    }
    
    if (shell->import_buffer) {
        free(shell->import_buffer);
        shell->import_buffer = NULL;
    }
    
    shell->import_buffer_size = vfs_read(shell->device_filesystem,
                                         argv[1],
                                         &shell->import_buffer);
    
    printf("Read %d bytes from the device.\n", shell->import_buffer_size);
}
