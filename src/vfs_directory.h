#ifndef VFS_DIRECTORY
#define VFS_DIRECTORY

#include <stdint.h>
#include "vfs_node.h"
#include "vfs.h"

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
