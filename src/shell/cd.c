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

#include <shell/cd.h>
#include <shell/shell.h>

#include <vfs/vfs.h>
#include <vfs/node.h>

int shell_cd(shell_t shell, int argc, const char *argv[])
{
    assert(shell);

    vfs_t fs = shell->device_filesystem;

    // Ignore all arguments. We don't need them.
    vfs_node_t dir = vfs_get_directory(fs);
    if (!dir || !(dir && dir->attributes & vfs_node_directory_attribute)) {
        fprintf(stderr, "Unknown error occured\n");
        return SHELL_ERROR_CODE;
    }
    
    // Look up the requested node in the directory. If it exists ensure it
    // is a directory.
    vfs_node_t node = fs->filesystem_interface->get_node(dir, argv[1]);
    if (!node || !(node && node->attributes & vfs_node_directory_attribute)) {
        fprintf(stderr, "Attempted to access a none directory node\n");
        return SHELL_ERROR_CODE;
    }

    // Set the directory.
    fs->filesystem_interface->set_directory(fs, node);
    
    return SHELL_OK;
}
