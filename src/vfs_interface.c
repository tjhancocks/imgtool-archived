#include <stdlib.h>
#include "vfs_interface.h"


vfs_interface_t vfs_interface_init()
{
    return calloc(1, sizeof(struct vfs_interface));
}

void vfs_interface_destroy(vfs_interface_t interface)
{
    free(interface);
}
