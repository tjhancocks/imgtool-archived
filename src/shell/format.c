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
#include <string.h>
#include <stdlib.h>

#include <shell/format.h>
#include <shell/shell.h>

#include <vfs/vfs.h>
#include <vfs/interface.h>
#include <common/host.h>

size_t data_size = 0;
uint8_t *data = NULL;

uint8_t *shell_format_import(const char *path)
{
    assert(path);
    FILE *f = fopen(path, "r");
    if (!f) {
        fprintf(stderr, "Could not open the specified file: %s\n", path);
        return NULL;
    }

    fseek(f, 0L, SEEK_END);
    data_size = (uint32_t)ftell(f);
    fseek(f, 0L, SEEK_SET);

    data = calloc(data_size, sizeof(uint8_t));
    fread(data, sizeof(uint8_t), data_size, f);
    fclose(f);

    return data;
}

int shell_format(struct shell *shell, int argc, const char *argv[])
{
    assert(shell);

    if (argc < 2 && argc > 4) {
        fprintf(stderr, "Expected at least 1 argument for the file system.\n");
        return SHELL_ERROR_CODE;
    }

    // Setup a temporary file system object that can be used to initialise
    // the device
    vfs_interface_t fs = vfs_interface_for(argv[1]);
    if (!fs) {
        fprintf(stderr, "Unrecognised file system type: %s\n", argv[1]);
        return SHELL_ERROR_CODE;
    }

    // Was there a bootsector specified?
    uint8_t *bootsector = NULL;
    if (argc >= 3) {
        const char *path = host_expand_path(argv[2]);
        bootsector = shell_format_import(path);
        if (data_size != shell->attached_device->sector_size) {
            fprintf(stderr, "Bootsector is the wrong size!\n");
            return SHELL_ERROR_CODE;
        }
        free((void *)path);
    }

    // Was there a reserved range of sectors specified?
    size_t reserved_size = 0;
    uint8_t *reserved = NULL;
    if (argc == 4) {
        const char *path = host_expand_path(argv[3]);
        reserved = shell_format_import(path);
        reserved_size = data_size;
        reserved_size = (reserved_size >> 9) + ((reserved_size & 0x1ff) ? 1 : 0);
        printf("reserved size: %zu\n", reserved_size);
        free((void *)path);
    }

    // Format the device. No label or bootcode here.
    fs->format_device(
        shell->attached_device, 
        NULL,
        bootsector, 
        reserved, 
        reserved_size
    );

    // Clean up
    vfs_interface_destroy(fs);
    free(reserved);
    free(bootsector);
    data = NULL;
    
    return SHELL_OK;
}
