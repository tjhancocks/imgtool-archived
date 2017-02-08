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

#ifndef VFS
#define VFS

#include <device/virtual.h>
#include <vfs/interface.h>

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
void vfs_mkdir(vfs_t vfs, const char *name);

void vfs_write(vfs_t vfs, const char *name, uint8_t *bytes, uint32_t size);

#endif /* vfs_h */
