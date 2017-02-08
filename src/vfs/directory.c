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
#include <string.h>
#include <vfs/directory.h>


vfs_directory_t vfs_directory_init(vfs_t fs, void *info)
{
    vfs_directory_t dir = calloc(1, sizeof(*dir));
    dir->assoc_info = info;
    dir->fs = fs;
    return dir;
}

void vfs_directory_destroy(vfs_directory_t dir)
{
    if (dir) {
        vfs_node_t node = dir->first;
        while (node) {
            vfs_node_t tmp = node;
            node = node->next;
            vfs_node_destroy(tmp);
        }
        free(dir->assoc_info);
    }
    free(dir);
}

vfs_node_t vfs_directory_find(vfs_directory_t dir, const char *name)
{
    vfs_node_t node = dir->first;
    while (node) {
        if (strcmp(node->name, name) == 0) {
            break;
        }
        node = node->next;
    }
    return node;
}
