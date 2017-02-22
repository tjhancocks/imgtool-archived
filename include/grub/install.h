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

#define SECTOR_SIZE                     0x200
#define GRUB_STAGE_1_FILE               "stage1"
#define GRUB_STAGE_2_FILE               "stage2"

// The GRUB compatibility version information. This will be used to work out
// if the supplied GRUB binaries are compatible with what this tool will do.
#define COMPAT_VERSION_MAJOR            3
#define COMPAT_VERSION_MINOR            2
#define COMPAT_VERSION                  ((COMPAT_VERSION_MINOR << 8) \
                                        | COMPAT_VERSION_MAJOR)

// The following are a series of offsets in STAGE1. These instruct the tool on
// where to look for and where to place information whilst installing GRUB.
#define STAGE1_BPB_START                0x03
#define STAGE1_BPB_END                  0x3e
#define STAGE1_BPB_LEN                  (STAGE1_BPB_END - STAGE1_BPB_START)
#define STAGE1_VERS_OFFS                0x3e
#define STAGE1_BOOT_DRIVE               0x40
#define STAGE1_FORCE_LBA                0x41
#define STAGE1_STAGE2_ADDRESS           0x42
#define STAGE1_STAGE2_SECTOR            0x44
#define STAGE1_STAGE2_SEGMENT           0x48
#define STAGE1_BOOT_DRIVE_MASK          0x4d
#define STAGE1_WINDOWS_NT_MAGIC         0x1b8
#define STAGE1_PART_START               0x1be
#define STAGE1_PART_END                 0x1fe
#define BOOTSEC_SIG_OFFSET              0x1fe
#define BOOT_SIGNATURE                  0xaa55

// The next group are a series of offsets in STAGE2.
#define STAGE2_VERS_OFFS                0x206
#define STAGE2_INSTALLPART              0x208
#define STAGE2_SAVED_ENT                0x20c
#define STAGE2_ID                       0x210
#define STAGE2_FORCE_LBA                0x211
#define STAGE2_VERS_STR                 0x212

// GRUB Binary Identifiers
#define GRUB_ID_STAGE2                  0

#define GRUB_INCOMPATIBLE_ERROR         0x05
#define GRUB_FILE_ERROR                 0x04
#define GRUB_WONT_FIT_ERROR             0x03
#define GRUB_MEMORY_ALIGN_ERROR         0x02
#define GRUB_ERROR                      0x01
#define GRUB_OK                         0x00

struct grub_configuration {
    const char *source_path;
    const char *install_path;
    const char *configuration_path;
    const char *os_name;
    const char *root_name;
    const char *kernel_path;
};

struct grub_installation_info {
    struct grub_configuration cfg;
    struct vfs *fs;

    // GRUB Sources Information
    const char *stage2_os_file;
    const char *stage1_os_file;
    const char *stage2_file;
    const char *stage1_file;
    const char *config_file_location;

    // GRUB Buffers
    size_t stage1_size;
    unsigned char *stage1_buffer;
    size_t stage2_size;
    unsigned char *stage2_buffer;
    unsigned int stage2_first_sector;
    unsigned int stage2_second_sector;

    // Configuration Options
    unsigned int installaddr;
    unsigned int installlist;
    int is_stage_1_5;
    int is_open;
    int is_force_lba;
    int last_length;
    int saved_sector;
};
typedef struct grub_installation_info *grub_installation_info_t;

int grub_install(struct vfs *fs, struct grub_configuration cfg);


#endif
