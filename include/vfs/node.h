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
#include <time.h>

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
    // File System Related
    struct vfs *fs;
    
    // Directory Tree Related
    struct vfs_node *prev_sibling;
    struct vfs_node *next_sibling;
    
    // Node Name
    const char *name;
    
    // Node Attribute and Calculated Characteristics
    enum vfs_node_attributes attributes;
    enum vfs_node_state state;
    
    // Node Meta Data
    uint32_t size;
    time_t creation_time;
    time_t modification_time;
    time_t access_time;
    
    // Source Information
    void *assoc_info;
    
    // Editing
    uint8_t is_dirty:1;
    uint8_t reserved:7;
};
typedef struct vfs_node * vfs_node_t;


vfs_node_t vfs_node_init(struct vfs *fs,
                         const char *name,
                         enum vfs_node_attributes attributes,
                         enum vfs_node_state state,
                         void *node_info);

void vfs_node_destroy(vfs_node_t node);

int vfs_node_test_attribute(vfs_node_t node, enum vfs_node_attributes attr);
void vfs_node_set_attribute(vfs_node_t node, enum vfs_node_attributes attr);
void vfs_node_unset_attribute(vfs_node_t node, enum vfs_node_attributes attr);

void vfs_node_set_size(vfs_node_t node, uint32_t size);

void vfs_node_update_modification_time(vfs_node_t node);
void vfs_node_update_access_time(vfs_node_t node);


#endif
