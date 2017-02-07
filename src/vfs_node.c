#include <stdlib.h>
#include <string.h>
#include "vfs_node.h"

vfs_node_t vfs_node_init(vfs_t fs, void *info)
{
    vfs_node_t node = calloc(1, sizeof(*node));

    node->assoc_info = info;
    node->fs = fs;

    return node;
}

void vfs_node_destroy(vfs_node_t node)
{
    if (node) {
        free((void *)node->name);
        free(node->assoc_info);
    }
    free(node);
}

vfs_node_t vfs_node_append_node(vfs_node_t head, vfs_node_t subject)
{
    vfs_node_t node = head;

    while (node && node->next) {
        node = node->next;
    }

    if (head == NULL) {
        head = subject;
    }
    else if (node) {
        node->next = subject;
    }

    return head;
}
