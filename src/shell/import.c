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
#include <stdio.h>

#include <shell/import.h>
#include <shell/shell.h>
#include <common/host.h>

void shell_import(struct shell *shell, int argc, const char *argv[])
{
    assert(shell);

    if (argc != 2) {
        fprintf(stderr, "Expected a single argument for the path to import.\n");
        return;
    }

    if (shell->import_buffer) {
        free(shell->import_buffer);
        shell->import_buffer = NULL;
    }

    // Open the file, get the size and read the data
    const char *path = host_expand_path(argv[1]);
    FILE *f = fopen(path, "r");
    fseek(f, 0L, SEEK_END);
    shell->import_buffer_size = (uint32_t)ftell(f);
    fseek(f, 0L, SEEK_SET);
    free((void *)path);

    shell->import_buffer = calloc(shell->import_buffer_size, sizeof(uint8_t));
    fread(shell->import_buffer, sizeof(uint8_t), shell->import_buffer_size, f);

    printf("Imported %d bytes to internal buffer\n", shell->import_buffer_size);
}
