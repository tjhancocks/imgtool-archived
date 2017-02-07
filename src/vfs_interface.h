#include "vdevice.h"

#ifndef VFS_INTERFACE
#define VFS_INTERFACE

struct vfs;

struct vfs_interface {
    const char *(*type_name)();
    void (*format_device)(struct vfs *fs, const char *name, uint8_t *bootcode);
    void *(*mount_filesystem)(struct vfs *fs);
    void (*unmount_filesystem)(struct vfs *fs);
    void (*change_directory)(struct vfs *fs, void *);
    void *(*list_directory)(struct vfs *fs);
    void (*touch)(struct vfs *fs, const char *name);
    void (*write)(struct vfs *fs, const char *name, uint8_t *bytes, uint32_t n);
};

typedef struct vfs_interface * vfs_interface_t;

vfs_interface_t vfs_interface_init();
void vfs_interface_destroy(vfs_interface_t interface);

#endif
