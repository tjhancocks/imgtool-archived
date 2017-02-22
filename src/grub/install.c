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

#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <string.h>

#include <grub/install.h>
#include <device/virtual.h>
#include <vfs/vfs.h>
#include <common/host.h>

#define GRUB_PATH_LENGTH_MAX 1024

#pragma mark - Helpers

const char *grub_os_path_get(const char *grub_path, const char *file)
{
    char *path = calloc(GRUB_PATH_LENGTH_MAX, sizeof(*path));
    const char *expanded_grub_path = host_expand_path(grub_path);
    
    // Copy the expanded path into the path
    size_t len = strlen(expanded_grub_path);
    memcpy(path, expanded_grub_path, len);
    
    // If the last character is '/' then truncate it.
    if (path[len - 1] == '/') {
        path[--len] = '\0';
    }
    
    // The next part is the sub directory in which the GRUB files should
    // be located. Add this to the path.
    const char *grub_sub_path = "/boot/grub/";
    strcat(path, grub_sub_path);
    
    // Finally we need to add the file that we're actually looking for.
    strcat(path, file);
    
    // Clean up
    free((void *)expanded_grub_path);
    return path;
}

void *grub_read_file(const char *filepath, size_t *size)
{
    FILE *fp = fopen(filepath, "rb");
    if (!fp) {
        fprintf(stderr, "Failed to read GRUB file: %s\n", filepath);
        return NULL;
    }
    
    // Determine how long the file is.
    fseek(fp, 0L, SEEK_END);
    size_t n = (size_t)ftell(fp);
    fseek(fp, 0L, SEEK_SET);
    
    // Allocate a buffer for the file to be read into.
    void *buffer = calloc(n, sizeof(uint8_t));
    fread(buffer, sizeof(uint8_t), n, fp);
    
    // Send the size to the caller
    if (size) {
        *size = n;
    }
    
    // Clean up
    fclose(fp);
    return buffer;
}


#pragma mark - GRUB Helpers

int grub_disk_read_savesect_func(grub_installation_info_t info,
                                  int sector,
                                  int offset,
                                  int length)
{
    if (offset != 0 || length != SECTOR_SIZE) {
        return GRUB_MEMORY_ALIGN_ERROR;
    }

    info->saved_sector = sector;

    return GRUB_OK;
}

int grub_disk_read_blocklist_func(grub_installation_info_t info,
                                   int sector,
                                   int offset,
                                   int length)
{
    if (offset != 0 || info->last_length != SECTOR_SIZE) {
        return GRUB_MEMORY_ALIGN_ERROR;
    }

    info->last_length = length;

    if (*((unsigned long *)(uintptr_t)info->installlist) != sector ||
        info->installlist == (int)info->stage2_buffer + SECTOR_SIZE + 4)
    {
        info->installlist -= 8;

        if (*((unsigned long *)((uintptr_t)info->installlist - 8))) {
            return GRUB_WONT_FIT_ERROR;
        }
        else {
            *((unsigned short *)((uintptr_t)info->installlist + 2)) =
                (info->installaddr >> 4);
            *((unsigned long *)((uintptr_t)info->installlist - 4)) = sector;
        }
    }

    *((unsigned short *)(uintptr_t)info->installlist) += 1;
    info->installaddr += SECTOR_SIZE;

    return GRUB_OK;
}


#pragma mark - Stage 1

int grub_stage1_preparation(grub_installation_info_t info)
{
    // Load the stage1 binary
    info->stage1_buffer = grub_read_file(info->stage1_os_file,
                                         &info->stage1_size);
    info->is_open = (info->stage1_buffer != NULL);
    if (!info->is_open) {
        return GRUB_FILE_ERROR;
    }

    // We're going to get the bootsector of the current device and copy over
    // the BIOS Parameter Block for it.
    unsigned char *buffer = device_read_sector(info->fs->device, 0);
    memcpy(info->stage1_buffer + STAGE1_BPB_START,
           buffer + STAGE1_BPB_START,
           STAGE1_BPB_LEN);

    // Check if we're operating on a hard drive. If we are, copy the MBR as
    // well.
    if (info->fs->device->media == vmedia_hard_disk) {
        memcpy(info->stage1_buffer + STAGE1_WINDOWS_NT_MAGIC,
               buffer + STAGE1_WINDOWS_NT_MAGIC,
               STAGE1_PART_END - STAGE1_WINDOWS_NT_MAGIC);
    }

    // Check the version and signature of stage for to ensure compatibility.
    uintptr_t ptr = (uintptr_t)info->stage1_buffer;
    if (*((short *)(ptr + STAGE1_VERS_OFFS)) != COMPAT_VERSION ||
        (*((unsigned short *)(ptr + BOOTSEC_SIG_OFFSET)) != BOOT_SIGNATURE))
    {
        free((void *)buffer);
        return GRUB_INCOMPATIBLE_ERROR;
    }
    
    // If we're on a floppy disk then there must be an iteration probe?
    if (!(info->fs->device->media == vmedia_floppy) &&
        (*((unsigned char *)(ptr + STAGE1_PART_START)) == 0x80 ||
        info->stage1_buffer[STAGE1_PART_START] == 0))
    {
        return GRUB_INCOMPATIBLE_ERROR;
    }

    // Clean up
    free((void *)buffer);
    return GRUB_OK;
}


#pragma mark - Stage 2

int grub_stage2_preparation(grub_installation_info_t info)
{
    // Load the stage2 binary, and copy it into the device.
    info->stage2_buffer = grub_read_file(info->stage2_os_file,
                                         &info->stage2_size);
    info->is_open = (info->stage2_buffer != NULL);
    if (!info->is_open) {
        return GRUB_FILE_ERROR;
    }

    vfs_write(info->fs,
              info->stage2_file,
              info->stage2_buffer,
              (uint32_t)info->stage2_size);
    
    // Generate some easy access references
    vfs_t fs = info->fs;
    uintptr_t ptr = (uintptr_t)info->stage1_buffer;
    
    // Set the bootdrive in stage1 and whether or not force lba should be
    // enabled or not.
    // TODO: Handle the force lba option.
    *((unsigned char *)(ptr + STAGE1_BOOT_DRIVE)) = fs->device->media;
    *((unsigned char *)(ptr + STAGE1_FORCE_LBA)) = 0;

    *((unsigned short *)(ptr + STAGE1_BOOT_DRIVE_MASK))
        = (fs->device->media & 0x80);

    
    // Get the first sector of stage2 and keep track of it.
    info->stage2_first_sector = vfs_nth_sector_of(fs, 0, info->stage2_file);
    info->stage2_second_sector = vfs_nth_sector_of(fs, 1, info->stage2_file);
    
    // Check the version of stage2
    uintptr_t s2ptr = (uintptr_t)info->stage2_buffer;
    if (*((short *)(s2ptr + STAGE2_VERS_OFFS)) != COMPAT_VERSION) {
        return GRUB_INCOMPATIBLE_ERROR;
    }

    // Check the ID of stage 2
    if (*((unsigned char *)(s2ptr + STAGE2_ID)) != GRUB_ID_STAGE2) {
        info->is_stage_1_5 = 1;
    }

    // Determine what the install address should be.
    if (info->is_stage_1_5) {
        info->installaddr = 0x2000;
    }
    else {
        info->installaddr = 0x8000;
    }

    // Write location information for stage2 into stage1.
    *((unsigned long *)(ptr +STAGE1_STAGE2_SECTOR)) = info->stage2_first_sector;
    *((unsigned short *)(ptr +STAGE1_STAGE2_ADDRESS)) = info->installaddr;
    *((unsigned short *)(ptr +STAGE1_STAGE2_SEGMENT)) = info->installaddr >> 4;

    // Not entirely sure the purpose of this next section. It appears to be
    // the sector/block chain for stage2 so that it knows how to load
    // everything.
    uintptr_t i = (uintptr_t)info->stage2_buffer + SECTOR_SIZE - 4;
    while (*((unsigned long *) i)) {
        if (i < (uintptr_t)info->stage2_buffer
            || (*((int *)(i - 4)) & 0x80000000)
            || *((unsigned short *) i) >= 0xa00
            || *((short *)(i + 2)) == 0)
        {
            return GRUB_INCOMPATIBLE_ERROR;
        }

        *((int *) i) = 0;
        *((int *)(i - 4)) = 0;
        i -= 8;
    }

    info->installlist = (unsigned int)s2ptr + SECTOR_SIZE + 4;
    info->installaddr += SECTOR_SIZE;

    // Write out all the sectors
    uint32_t sectors =  vfs_sector_count_of(info->fs, info->stage2_file);
    for (uint32_t i = 0; i < sectors; ++i) {
        uint32_t sector = vfs_nth_sector_of(fs, i, info->stage2_file);
        grub_disk_read_blocklist_func(info, sector, i*SECTOR_SIZE, SECTOR_SIZE);
    }

    // Find the string for the configuration file name.
    info->config_file_location = (const char *)s2ptr + STAGE2_VERS_OFFS;
    while (*(info->config_file_location++));

    // TODO: Add the configuration setup here...

    //
    
    
    return GRUB_OK;
}


#pragma mark - Main Installation

int grub_install(struct vfs *fs, struct grub_configuration cfg)
{
    int err = GRUB_OK;

    // Construct an installation object for GRUB. This will keep all our working
    // data so that it can be passed around.
    grub_installation_info_t grub = calloc(1, sizeof(*grub));
    grub->cfg = cfg;
    grub->fs = fs;

    // Process all options and work out exactly what we're going to try and
    // build here.
    grub->stage1_os_file = grub_os_path_get(cfg.source_path, GRUB_STAGE_1_FILE);
    grub->stage2_os_file = grub_os_path_get(cfg.source_path, GRUB_STAGE_2_FILE);
    grub->stage2_file = GRUB_STAGE_2_FILE;

    // Move into the installation directory in preparation.
    vfs_mkdir(fs, grub->cfg.install_path);
    vfs_navigate_to_path(fs, grub->cfg.install_path);

    // Do all preparatory work on the files ready for copying them to disk.
    err = grub_stage1_preparation(grub);
    if (err == GRUB_OK) {
        err = grub_stage2_preparation(grub);
    }

    // Flush changes to the disk.
    if (err == GRUB_OK) {
        vdevice_t dev = fs->device;
        device_write_sector(dev, 0, grub->stage1_buffer);

        uint32_t sector1 = vfs_nth_sector_of(fs, 0, grub->stage2_file);
        device_write_sector(dev, sector1, grub->stage2_buffer);

        uint32_t sector2 = vfs_nth_sector_of(fs, 1, grub->stage2_file);
        device_write_sector(dev, sector2, grub->stage2_buffer + SECTOR_SIZE);
    }

    // Clean up GRUB installation information.
    free((void *)grub->stage1_os_file);
    free((void *)grub->stage2_os_file);
    free((void *)grub->stage1_buffer);
    free((void *)grub->stage2_buffer);
    free(grub);

    // At this point report everything as being successful.
    return err;
}
