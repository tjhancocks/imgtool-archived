#include <stdlib.h>
#include <string.h>
#include "vfs_directory.h"


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
