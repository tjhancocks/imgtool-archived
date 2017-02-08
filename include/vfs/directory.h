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

#ifndef VFS_DIRECTORY
#define VFS_DIRECTORY

#include <stdint.h>
#include <vfs/node.h>
#include <vfs/vfs.h>

// The Virtual File System Directory is an abstract structure that represents
// a directory on a concrete file system. It contains the head node of the
// directory as well as associated information.
struct vfs_directory {
    vfs_t fs;
    void *assoc_info;
    uint32_t max_nodes;
    vfs_node_t first;
};

typedef struct vfs_directory * vfs_directory_t;


// Construct a new VFS Directory instance with the specified associated
// information and virtual file system.
vfs_directory_t vfs_directory_init(vfs_t fs, void *info);

// Destroy the specified VFS Directory.
void vfs_directory_destroy(vfs_directory_t dir);

// Find the node with the specified name. Returns NULL if absent.
vfs_node_t vfs_directory_find(vfs_directory_t dir, const char *name);

#endif
