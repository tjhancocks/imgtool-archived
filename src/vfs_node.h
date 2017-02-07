#ifndef VFS_NODE
#define VFS_NODE

#include <stdint.h>
#include "vfs.h"

struct vfs_node;

enum vfs_node_state {
    vfs_node_unused = 0,
    vfs_node_available = 1,
    vfs_node_used = 2,
};

struct vfs_node {
    vfs_t fs;
    void *assoc_info;
    uint8_t is_dirty;
    uint8_t is_directory;
    uint8_t is_hidden;
    uint8_t is_system;
    uint8_t is_readonly;
    const char *name;
    uint32_t size;
    enum vfs_node_state state;
    struct vfs_node *next;
};

typedef struct vfs_node * vfs_node_t;

vfs_node_t vfs_node_init(vfs_t fs, void *info);
void vfs_node_destroy(vfs_node_t node);

vfs_node_t vfs_node_append_node(vfs_node_t head, vfs_node_t subject);

#endif
