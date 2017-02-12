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

#ifndef GRUB_INSTALL
#define GRUB_INSTALL

struct vfs;

// The GRUB compatibility version information. This will be used to work out
// if the supplied GRUB binaries are compatible with what this tool will do.
#define GRUB_COMPAT_VERSION_MAJOR           3
#define GRUB_COMPAT_VERSION_MINOR           2
#define GRUB_COMPAT_VERSION                 ((GRUB_COMPAT_VERSION_MAJOR << 8) \
                                            | GRUB_COMPAT_VERSION_MINOR)


// The following are a series of offsets in STAGE1. These instruct the tool on
// where to look for and where to place information whilst installing GRUB.
#define GRUB_STAGE1_VERS_OFFS               0x3e
#define GRUB_STAGE1_BOOT_DRIVE              0x40
#define GRUB_STAGE1_FORCE_LBA               0x41
#define GRUB_STAGE1_STAGE2_ADDRESS          0x42
#define GRUB_STAGE1_STAGE2_SECTOR           0x44
#define GRUB_STAGE1_STAGE2_SEGMENT          0x48

struct grub_configuration {
    const char *source_path;
    const char *install_path;
    const char *configuration_path;
    const char *os_name;
    const char *root_name;
    const char *kernel_path;
};

int grub_install(struct vfs *fs, struct grub_configuration cfg);


#endif
