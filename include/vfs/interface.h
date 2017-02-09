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

#include <device/virtual.h>

#ifndef VFS_INTERFACE
#define VFS_INTERFACE

struct vfs;

struct vfs_interface {
    const char *(*type_name)();
    void (*format_device)(vdevice_t dev, const char *name, uint8_t *bootcode);
    void *(*mount_filesystem)(struct vfs *fs);
    void (*unmount_filesystem)(struct vfs *fs);
    void (*change_directory)(struct vfs *fs, void *);
    void *(*list_directory)(struct vfs *fs);
    void (*touch)(struct vfs *fs, const char *name);
    void (*write)(struct vfs *fs, const char *name, uint8_t *bytes, uint32_t n);
    void (*mkdir)(struct vfs *fs, const char *name);
};

typedef struct vfs_interface * vfs_interface_t;

vfs_interface_t vfs_interface_for_device(vdevice_t dev);
vfs_interface_t vfs_interface_for(const char *type);

vfs_interface_t vfs_interface_init();
void vfs_interface_destroy(vfs_interface_t interface);

#endif
