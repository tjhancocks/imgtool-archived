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

#include <stdint.h>

#ifndef FAT12_STRUCTURES
#define FAT12_STRUCTURES

struct fat12_bpb {
    uint8_t jmp[3];
    uint8_t oem[8];
    uint16_t bytes_per_sector;
    uint8_t sectors_per_cluster;
    uint16_t reserved_sectors;
    uint8_t table_count;
    uint16_t directory_entries;
    uint16_t total_sectors_16;
    uint8_t media_type;
    uint16_t sectors_per_fat;
    uint16_t sectors_per_track;
    uint16_t heads;
    uint32_t hidden_sectors;
    uint32_t total_sectors_32;
    uint8_t drive;
    uint8_t nt_reserved;
    uint8_t signature;
    uint32_t volume_id;
    uint8_t label[11];
    uint8_t system_id[8];
    uint8_t boot_code[448];
    uint16_t boot_signature;
} __attribute__((packed));
typedef struct fat12_bpb * fat12_bpb_t;


struct fat12_sfn {
    uint8_t name[11];
    uint8_t attribute;
    uint8_t nt_reserved;
    uint8_t ctime_ms;
    uint16_t ctime;
    uint16_t cdate;
    uint16_t adate;
    uint16_t unused;
    uint16_t mtime;
    uint16_t mdate;
    uint16_t first_cluster;
    uint32_t size;
} __attribute__((packed));
typedef struct fat12_sfn * fat12_sfn_t;


struct fat12_directory_info {
    // The following fields define the location and size of the directory
    // contents.
    uint16_t first_cluster;
    uint32_t starting_sector;
    uint32_t sector_count;
};
typedef struct fat12_directory_info * fat12_directory_info_t;

struct vfs_directory;

struct fat12 {
    fat12_bpb_t bpb;
    uint8_t *fat_data;
    struct vfs_node *root_directory;
    struct vfs_node *working_directory;
};
typedef struct fat12 * fat12_t;

#endif
