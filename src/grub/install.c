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

const char *grub_path_get(const char *grub_path, const char *file)
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


#pragma mark - Compatibility Testing

int grub_test_compatibility(grub_installation_info_t grub)
{
    // We're expecting the size to be 512 bytes.
    if (!grub->stage1_buffer || grub->stage1_size != 512) {
        return GRUB_ERROR;
    }

    // We're expecting a copy of stage 2 to exist.
    if (!grub->stage2_buffer) {
        return GRUB_ERROR;
    }
    
    // Get the compatibility version from the stage 1 data and compare it to
    // the value baked into the tool.
    uint8_t s1_major = grub->stage1_buffer[GRUB_STAGE1_VERS_OFFS];
    uint8_t s1_minor = grub->stage1_buffer[GRUB_STAGE1_VERS_OFFS + 1];
    if (s1_major != GRUB_COMPAT_VERSION_MAJOR &&
        s1_minor != GRUB_COMPAT_VERSION_MINOR)
    {
        return GRUB_ERROR;
    }

    // Check the compatibility version from stage 2.
    uint8_t s2_major = grub->stage2_buffer[GRUB_STAGE2_VERS_OFFS];
    uint8_t s2_minor = grub->stage2_buffer[GRUB_STAGE2_VERS_OFFS + 1];
    if (s2_major != GRUB_COMPAT_VERSION_MAJOR &&
        s2_minor != GRUB_COMPAT_VERSION_MINOR)
    {
        return GRUB_ERROR;
    }

    // Check the stage 2 id and ensure it is a valid stage 2 binary
    uint8_t s2_id = grub->stage2_buffer[GRUB_STAGE2_ID];
    if (s2_id != GRUB_ID_STAGE2) {
        return GRUB_ERROR;
    }
    
    // At this point assume its compatible
    return GRUB_OK;
}


#pragma mark - Stage 1

void grub_read_stage1(grub_installation_info_t grub)
{
    const char *stage1_path = grub_path_get(grub->cfg.source_path, "stage1");
    grub->stage1_buffer = grub_read_file(stage1_path, &grub->stage1_size);
}

void grub_specify_boot_drive(grub_installation_info_t grub, enum vmedia_type m)
{
    grub->stage1_buffer[GRUB_STAGE1_BOOT_DRIVE] = (m & 0xFF);
}

void grub_force_lba(grub_installation_info_t grub)
{
    grub->stage1_buffer[GRUB_STAGE1_FORCE_LBA] = 0x01;
}

void grub_enable_boot_drive_workaround(grub_installation_info_t grub)
{
    grub->stage1_buffer[GRUB_STAGE1_BOOT_DRIVE_CHK] = 0x90;
    grub->stage1_buffer[GRUB_STAGE1_BOOT_DRIVE_CHK + 1] = 0x90;
}


#pragma mark - Stage 2

void grub_read_stage2(grub_installation_info_t grub)
{
    const char *stage2_path = grub_path_get(grub->cfg.source_path, "stage2");
    grub->stage2_buffer = grub_read_file(stage2_path, &grub->stage2_size);
}

void grub_put_stage2(grub_installation_info_t grub)
{
    // This is going to involve some acrobatics. We need to ensure we have all
    // the structure setup for GRUB. Get to the root directory.
    vfs_t fs = grub->fs;
    vfs_navigate_to_path(fs, "/");

    // We have a path for the installation destination of GRUB in the info.
    // Try and make the directory and navigate there.
    vfs_mkdir(fs, grub->cfg.install_path);
    vfs_navigate_to_path(fs, grub->cfg.install_path);
    
    // Write Stage2 to the directory.
    vfs_write(fs, "STAGE2", grub->stage2_buffer, (uint32_t)grub->stage2_size);
}

void grub_prepare_stage2(grub_installation_info_t grub)
{
    grub_specify_boot_drive(grub, grub->fs->device->media);
    //grub_force_lba(grub);

    // If the media is a hard disk then enable a work around in GRUB. According
    // to GRUB source code it is for buggy BIOSes which don't correctly report
    // the boot drive
    if (grub->fs->device->media == vmedia_hard_disk) {
        grub_enable_boot_drive_workaround(grub);
    }

    // Get the basic GRUB infrastructure onto the disk. This will help us
    // identifying values that are required.
    grub_put_stage2(grub);

    // Identify the installation address.
    grub->installaddr = 0x8000;

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

    // Read in GRUB Stage 1 and 2 if the are available.
    grub_read_stage1(grub);
    grub_read_stage2(grub);

    // The very first thing we need to do here is to confirm that we have a
    // copy of GRUB available to us that provides the necessary components and
    // is compatible.
    if (!grub_test_compatibility(grub)) {
        fprintf(stderr, "A compatible version of GRUB was not found.\n");
        err = GRUB_ERROR;
        goto FINISH_GRUB_INSTALL;
    }
    printf("Identifier compatible version of GRUB\n");

    // Verify that we have a mount device available to us for use.
    if (!fs) {
        fprintf(stderr, "Mounted device not found. Unable to proceed.\n");
        err = GRUB_ERROR;
        goto FINISH_GRUB_INSTALL;
    }

    // We have at least a valid version of GRUB at our disposal.
    grub_prepare_stage2(grub);


FINISH_GRUB_INSTALL:
    // Clean up and release everything that has been gradually allocated.
    free(grub->stage1_buffer);
    free(grub->stage2_buffer);
    free(grub);

    // At this point report everything as being successful.
    return 1;
}
