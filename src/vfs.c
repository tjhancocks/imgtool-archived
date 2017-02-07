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
#include <assert.h>
#include "vfs.h"

vfs_t vfs_init(vdevice_t dev, vfs_interface_t interface)
{
    assert(dev);
    assert(interface);

    vfs_t vfs = calloc(1, sizeof(*vfs));
    vfs->device = dev;
    vfs->filesystem_interface = interface;
    vfs->type = interface->type_name();

    return vfs;
}

void vfs_destroy(vfs_t vfs)
{
    if (vfs) {
        vfs->filesystem_interface->unmount_filesystem(vfs);
        vfs_interface_destroy(vfs->filesystem_interface);
        device_destroy(vfs->device);
    }
    free(vfs);
}

int vfs_format_device(vfs_t vfs, const char *label)
{
    assert(vfs);
    
    if (vfs->assoc_info) {
        fprintf(stderr, "Unable to format device. Still mounted.\n");
        return 0;
    }
    
    vfs->filesystem_interface->format_device(vfs, label, NULL);
    return 1;
}

void vfs_mount(vfs_t vfs)
{
    assert(vfs);
    vfs->filesystem_interface->unmount_filesystem(vfs);
    vfs->assoc_info = vfs->filesystem_interface->mount_filesystem(vfs);
    
    if (!vfs->assoc_info) {
        fprintf(stderr, "Valid filesystem not found\n");
    }

    vfs->filesystem_interface->change_directory(vfs, NULL);
}

void vfs_unmount(vfs_t vfs)
{
    assert(vfs);
    vfs->filesystem_interface->unmount_filesystem(vfs);
}


const char *vfs_pwd(vfs_t vfs)
{
    assert(vfs);
    if (vfs->assoc_info) {
        return "/";
    }
    else {
        return "<unmounted>";
    }
}

struct vfs_directory *vfs_list_directory(vfs_t vfs)
{
    assert(vfs);
    if (vfs->assoc_info) {
        return vfs->filesystem_interface->list_directory(vfs);
    }
    return NULL;
}

void vfs_touch(vfs_t vfs, const char *name)
{
    assert(vfs);
    vfs->filesystem_interface->touch(vfs, name);
}

void vfs_write(vfs_t vfs, const char *name, uint8_t *bytes, uint32_t size)
{
    vfs_touch(vfs, name);
    vfs->filesystem_interface->write(vfs, name, bytes, size);
}
