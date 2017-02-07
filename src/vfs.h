#ifndef VFS
#define VFS

#include "vdevice.h"
#include "vfs_interface.h"

struct vfs_directory;

struct vfs {
    const char *type;
    void *assoc_info;
    vdevice_t device;
    vfs_interface_t filesystem_interface;
};

typedef struct vfs * vfs_t;

vfs_t vfs_init(vdevice_t dev, vfs_interface_t inteface);
void vfs_destroy(vfs_t vfs);

int vfs_format_device(vfs_t vfs, const char *label);
void vfs_mount(vfs_t vfs);
void vfs_unmount(vfs_t vfs);

const char *vfs_pwd(vfs_t vfs);

struct vfs_directory *vfs_list_directory(vfs_t vfs);

void vfs_touch(vfs_t vfs, const char *name);

void vfs_write(vfs_t vfs, const char *name, uint8_t *bytes, uint32_t size);

#endif /* vfs_h */
