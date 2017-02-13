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

#ifndef VFS_INTERFACE
#define VFS_INTERFACE

#include <device/virtual.h>
#include <vfs/node.h>

struct vfs;

struct vfs_interface {
    /// Reports the type name of the filesystem. This will be something like
    /// `fat12` or `ext2`, etc.
    const char *(*type_name)();
    
    /// Invokes a formatting of the device as the specified filesystem. A volume
    /// label and custom bootcode can be provided.
    void (*format_device)(vdevice_t dev, const char *name, uint8_t *bootcode);
    
    /// Mounts the filesystem in question. This action may vary from filesystem
    /// to filesystem, but will generally load all root directory metadata.
    void *(*mount_filesystem)(struct vfs *fs);
    
    /// Cleans up the virtual file system and destroys all concrete filesystem
    /// structures left in place.
    void (*unmount_filesystem)(struct vfs *fs);
    
    /// Sets the current working directory of the filesystem.
    void (*set_directory)(struct vfs *fs, vfs_node_t dir);
    
    /// Gets the current working directory of the filesystem.
    vfs_node_t (*get_directory_list)(struct vfs *fs);

    /// Locate the node with the specified name inside the specified directory.
    vfs_node_t (*get_node)(struct vfs *fs, const char *name);
    
    /// Create a new directory entry in the current working directory with the
    /// specified file name and attributes. This is absent any form of data.
    void (*create_file)(struct vfs *fs,
                        const char *filename,
                        enum vfs_node_attributes attributes);
    
    /// Create a new directory entry in the current working directory with the
    /// specified file name and attributes. This is to explicitly create a
    /// directory.
    void (*create_dir)(struct vfs *fs,
                       const char *name,
                       enum vfs_node_attributes a);
    
    /// Write the specified bytes of data to the named file.
    void (*write)(struct vfs *fs, const char *name, void *bytes, uint32_t n);
    
    /// Read the contents of the specified file. This returns the size of the
    /// file in bytes.
    uint32_t (*read)(struct vfs *fs, const char *name, void **bytes);
    
    /// Force all metadata in the current working directory to be flushed to
    /// disk.
    void (*flush_directory)(struct vfs *fs);
    
    /// Rename the specified entry in the current working directory.
    void (*rename)(struct vfs *fs, const char *old, const char *filename);
    
    /// Remove the specified enty in the current working directory.
    void (*remove)(struct vfs *fs, const char *filename);
};

typedef struct vfs_interface * vfs_interface_t;

vfs_interface_t vfs_interface_for_device(vdevice_t dev);
vfs_interface_t vfs_interface_for(const char *type);

vfs_interface_t vfs_interface_init();
void vfs_interface_destroy(vfs_interface_t interface);

#endif
