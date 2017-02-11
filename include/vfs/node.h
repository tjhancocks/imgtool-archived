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

#ifndef VFS_NODE
#define VFS_NODE

#include <stdint.h>

struct vfs;
struct vfs_node;

enum vfs_node_state {
    vfs_node_unused = 0,
    vfs_node_available = 1,
    vfs_node_used = 2,
};

enum vfs_node_attributes {
    vfs_node_read_only_attribute = 0x1,
    vfs_node_hidden_attribute = 0x2,
    vfs_node_system_attribute = 0x4,
    vfs_node_directory_attribute = 0x8,
};

struct vfs_node {
    struct vfs *fs;
    void *assoc_info;
    uint8_t is_dirty;
    const char *name;
    uint32_t size;
    enum vfs_node_attributes attributes;
    enum vfs_node_state state;
    struct vfs_node *next;
};

typedef struct vfs_node * vfs_node_t;

vfs_node_t vfs_node_init(struct vfs *fs, void *info);
void vfs_node_destroy(vfs_node_t node);

vfs_node_t vfs_node_append_node(vfs_node_t head, vfs_node_t subject);

#endif
