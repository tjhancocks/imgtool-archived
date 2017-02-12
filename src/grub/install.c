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

#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <string.h>

#include <grub/install.h>
#include <device/virtual.h>
#include <vfs/vfs.h>
#include <common/host.h>

#define GRUB_PATH_LENGTH_MAX 1024

#pragma mark - Helpers

const char *grub_path_get(const char *grub_path, const char *file)
{
    char *path = calloc(GRUB_PATH_LENGTH_MAX, sizeof(*path));
    const char *expanded_grub_path = host_expand_path(grub_path);
    
    // Copy the expanded path into the path
    size_t len = strlen(expanded_grub_path);
    memcpy(path, expanded_grub_path, len);
    
    // If the last character is '/' then truncate it.
    if (path[len - 1] == '/') {
        path[--len] = '\0';
    }
    
    // The next part is the sub directory in which the GRUB files should
    // be located. Add this to the path.
    const char *grub_sub_path = "/boot/grub/";
    strcat(path, grub_sub_path);
    
    // Finally we need to add the file that we're actually looking for.
    strcat(path, file);
    
    // Clean up
    free((void *)expanded_grub_path);
    return path;
}

void *grub_read_file(const char *filepath, size_t *size)
{
    FILE *fp = fopen(filepath, "rb");
    if (!fp) {
        fprintf(stderr, "Failed to read GRUB file: %s\n", filepath);
        return NULL;
    }
    
    // Determine how long the file is.
    fseek(fp, 0L, SEEK_END);
    size_t n = (size_t)ftell(fp);
    fseek(fp, 0L, SEEK_SET);
    
    // Allocate a buffer for the file to be read into.
    void *buffer = calloc(n, sizeof(uint8_t));
    fread(buffer, sizeof(uint8_t), n, fp);
    
    // Send the size to the caller
    if (size) {
        *size = n;
    }
    
    // Clean up
    fclose(fp);
    return buffer;
}


#pragma mark - Compatibility Testing

int grub_test_compatibility(const char *grub_path)
{
    const char *stage1_path = grub_path_get(grub_path, "stage1");
    
    // Load in the contents of Stage 1. If they exist then search for the
    // compatibility version information.
    size_t stage1_size = 0;
    uint8_t *stage1_data = grub_read_file(stage1_path, &stage1_size);
    
    // We're expecting the size to be 512 bytes.
    if (!stage1_data || stage1_size != 512) {
        free((void *)stage1_data);
        free((void *)stage1_path);
        return 0;
    }
    
    // Get the compatibility version from the stage 1 data and compare it to
    // the value baked into the tool.
    if (stage1_data[GRUB_STAGE1_VERS_OFFS] != GRUB_COMPAT_VERSION_MAJOR &&
        stage1_data[GRUB_STAGE1_VERS_OFFS + 1] != GRUB_COMPAT_VERSION_MINOR)
    {
        free((void *)stage1_data);
        free((void *)stage1_path);
        return 0;
    }
    
    // At this point assume its compatible
    free((void *)stage1_data);
    free((void *)stage1_path);
    return 1;
}


#pragma mark - Stage 1



#pragma mark - Stage 2



#pragma mark - Main Installation

int grub_install(struct vfs *fs, struct grub_configuration cfg)
{
    // The very first thing we need to do here is to confirm that we have a
    // copy of GRUB available to us that provides the necessary components and
    // is compatible.
    if (!grub_test_compatibility(cfg.source_path)) {
        fprintf(stderr, "A compatible version of GRUB was not found.\n");
        return 0;
    }
    
    return 1;
}
