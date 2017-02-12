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
#include <time.h>
#include <vfs/node.h>
#include <vfs/vfs.h>

vfs_node_t vfs_node_init(struct vfs *fs,
                         const char *name,
                         enum vfs_node_attributes attributes,
                         enum vfs_node_state state,
                         void *node_info,
                         void *dir_info)
{
    vfs_node_t node = calloc(1, sizeof(*node));
    
    node->fs = fs;
    node->attributes = attributes;
    node->state = state;
    node->assoc_node_info = node_info;
    node->assoc_directory_info = dir_info;
    
    size_t name_len = strlen(name);
    node->name = calloc(name_len + 1, sizeof(*node->name));
    strncpy((void *)node->name, name, name_len);
    
    return node;
}

void vfs_node_destroy(vfs_node_t node)
{
    if (node) {
        free((void *)node->name);
    }
    free(node);
}


#pragma mark - Children / Parent

void vfs_node_add_child(vfs_node_t parent, vfs_node_t child)
{
    if (!parent || !child) {
        return;
    }
    
    child->parent = parent;
    
    if (parent->last_child) {
        parent->last_child->next_sibling = child;
    }
    parent->last_child = child;
    
    if (!parent->first_child) {
        parent->first_child = child;
    }
}

void vfs_node_add_sibling(vfs_node_t subject, vfs_node_t sibling)
{
    if (!subject || !sibling) {
        return;
    }
    
    sibling->parent = subject->parent;
    
    vfs_node_t node = subject;
    while (node->next_sibling) {
        node = node->next_sibling;
    }
    node->next_sibling = sibling;
}


#pragma mark - Attribute Testing

int vfs_node_test_attribute(vfs_node_t node, enum vfs_node_attributes attr)
{
    return (node && node->attributes & attr);
}

void vfs_node_set_attribute(vfs_node_t node, enum vfs_node_attributes attr)
{
    if (node) {
        node->attributes |= attr;
    }
}

void vfs_node_unset_attribute(vfs_node_t node, enum vfs_node_attributes attr)
{
    if (node) {
        node->attributes &= ~attr;
    }
}


#pragma mark - Setters

void vfs_node_set_size(vfs_node_t node, uint32_t size)
{
    if (node) {
        node->size = size;
        node->is_dirty = 1;
    }
}

void vfs_node_update_modification_time(vfs_node_t node)
{
    if (node) {
        node->modification_time = time(NULL);
    }
}

void vfs_node_update_access_time(vfs_node_t node)
{
    if (node) {
        node->access_time = time(NULL);
    }
}

