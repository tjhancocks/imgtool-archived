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

#include <assert.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>


#include <fat/fat12.h>
#include <fat/fat12-structures.h>

#include <vfs/vfs.h>
#include <vfs/node.h>
#include <vfs/path.h>


#ifdef MAX
#   undef MAX
#endif

#define MAX(a,b) \
    ({ __typeof__ (a) _a = (a); \
       __typeof__ (b) _b = (b); \
       _a > _b ? _a : _b; })

#ifdef MIN
#   undef MIN
#endif

#define MIN(a,b) \
    ({ __typeof__ (a) _a = (a); \
       __typeof__ (b) _b = (b); \
       _a < _b ? _a : _b; })


#pragma mark - FAT Constants

enum fat12_attribute {
    fat12_attribute_readonly = 0x01,
    fat12_attribute_hidden = 0x02,
    fat12_attribute_system = 0x04,
    fat12_attribute_volume_id = 0x08,
    fat12_attribute_directory = 0x10,
    fat12_attribute_archive = 0x20,
};

enum fat12_cluster_ref {
    fat12_cluster_ref_free = 0x000,
    fat12_cluster_ref_eof = 0xfff,
    fat12_cluster_mask = 0xffff,
};


#pragma mark - VFS Interface (Prototypes)

const char *fat12_name();

void *fat12_mount(vfs_t fs);
void fat12_unmount(vfs_t fs);

void fat12_format_device(
    vdevice_t dev, 
    const char *label, 
    uint8_t *bootcode,
    uint8_t *reserved_data,
    uint16_t additional_reserved_sectors
);

vfs_node_t fat12_current_directory(vfs_t fs);
vfs_node_t fat12_get_directory_list(vfs_t fs);
void fat12_set_directory(vfs_t fs, vfs_node_t directory);

vfs_node_t fat12_get_node(vfs_t fs, const char *name);

vfs_node_t fat12_get_file(vfs_t fs,
                          const char *name,
                          uint8_t create_missing,
                          uint8_t creation_attributes);

void fat12_file_write(vfs_t fs, const char *name, void *data, uint32_t n);
uint32_t fat12_file_read(vfs_t fs, const char *name, void **data);

void fat12_create_file(vfs_t, const char *, enum vfs_node_attributes);
vfs_node_t fat12_create_dir(vfs_t fs,
                            const char *name,
                            enum vfs_node_attributes a);

void fat12_remove_file(vfs_t fs, const char *name);

void fat12_flush(vfs_t fs);


#pragma mark - VFS Interface Creation

vfs_interface_t fat12_init()
{
    vfs_interface_t fs = vfs_interface_init();

    fs->type_name = fat12_name;

    fs->mount_filesystem = fat12_mount;
    fs->unmount_filesystem = fat12_unmount;

    fs->format_device = fat12_format_device;

    fs->current_directory = fat12_current_directory;
    fs->get_directory_list = fat12_get_directory_list;
    fs->set_directory = fat12_set_directory;
    fs->get_node = fat12_get_node;

    fs->write = fat12_file_write;
    fs->read = fat12_file_read;

    fs->create_file = fat12_create_file;
    fs->create_dir = fat12_create_dir;
    
    fs->remove = fat12_remove_file;
    
    fs->flush_directory = fat12_flush;

    return fs;
}


#pragma mark - FAT Calculations

uint32_t fat12_fat_start(fat12_bpb_t bpb, uint8_t n)
{
    return bpb->reserved_sectors + (n * bpb->sectors_per_fat);
}

uint32_t fat12_fat_size(fat12_bpb_t bpb, uint32_t n)
{
    return bpb->sectors_per_fat * n;
}

uint32_t fat12_root_directory_start(fat12_bpb_t bpb)
{
    return fat12_fat_start(bpb, 0) + fat12_fat_size(bpb, bpb->table_count);
}

uint32_t fat12_root_directory_size(fat12_bpb_t bpb)
{
    return (((bpb->directory_entries * 32) + (bpb->bytes_per_sector - 1))
            / bpb->bytes_per_sector);
}

uint32_t fat12_data_start(fat12_bpb_t bpb)
{
    return fat12_root_directory_start(bpb) + fat12_root_directory_size(bpb);
}

uint32_t fat12_data_size(fat12_bpb_t bpb)
{
    uint32_t fat_size = fat12_fat_size(bpb, 1);
    uint32_t root_sectors = fat12_root_directory_size(bpb);
    return (bpb->total_sectors_16 - (bpb->reserved_sectors
                                     + (bpb->table_count * fat_size)
                                     + root_sectors));
}

uint32_t fat12_total_clusters(fat12_bpb_t bpb)
{
    return fat12_data_size(bpb) / bpb->sectors_per_cluster;
}

uint8_t fat12_translate_from_vfs_attributes(enum vfs_node_attributes vfsa)
{
    uint8_t attr = 0;
    attr |= vfsa & vfs_node_hidden_attribute ? fat12_attribute_hidden : 0;
    attr |= vfsa & vfs_node_read_only_attribute ? fat12_attribute_readonly : 0;
    attr |= vfsa & vfs_node_directory_attribute ? fat12_attribute_directory : 0;
    attr |= vfsa & vfs_node_system_attribute ? fat12_attribute_system : 0;
    return attr;
}

enum vfs_node_attributes fat12_translate_to_vfs_attributes(uint8_t attr)
{
    enum vfs_node_attributes vfsa = 0;
    vfsa |= attr & fat12_attribute_hidden ? vfs_node_hidden_attribute : 0;
    vfsa |= attr & fat12_attribute_readonly ? vfs_node_read_only_attribute : 0;
    vfsa |= attr & fat12_attribute_directory ? vfs_node_directory_attribute : 0;
    vfsa |= attr & fat12_attribute_system ? vfs_node_system_attribute : 0;
    return vfsa;
}


#pragma mark - FAT Date Calculations

uint16_t fat12_date_from_posix(time_t posix)
{
    struct tm ts = *localtime(&posix);
    
    uint32_t year = ((1900 + ts.tm_year) - 1980);
    uint32_t month = ts.tm_mon + 1;
    uint32_t day = ts.tm_mday;
    
    return ((year << 9) & 0xFE00) | ((month << 5) & 0x01E0) | (day & 0x001F);
}

uint16_t fat12_time_from_posix(time_t posix)
{
    struct tm ts = *localtime(&posix);
    
    uint32_t hour = ts.tm_hour;
    uint32_t min = ts.tm_min;
    uint32_t sec = ts.tm_sec;
    
    return ((hour << 11) & 0xF800) | ((min << 5) & 0x07E0) | (sec & 0x001F);
}

time_t fat12_date_time_to_posix(uint16_t date, uint16_t time)
{
    uint32_t year = (date & 0xFE00) >> 9;
    uint32_t month = (date & 0x01E0) >> 5;
    uint32_t day = (date & 0x1F);
    
    uint32_t hour = (time & 0xF800) >> 11;
    uint32_t minute = (time & 0x07E0) >> 5;
    uint32_t second = (time & 0x001F);
    
    struct tm ts;
    ts.tm_year = (year + 1980) - 1900;
    ts.tm_mon = month;
    ts.tm_mday = day;
    ts.tm_hour = hour;
    ts.tm_min = minute;
    ts.tm_sec = second;
    
    return mktime(&ts);
}


#pragma mark - FAT File System

const char *fat12_name()
{
    return "FAT12";
}

uint8_t fat12_test(vdevice_t dev, fat12_bpb_t *bpb_out)
{
    if (!dev) {
        return 0;
    }

    // Read in the boot sector and begin performing checks for the filesystem.
    // We're looking for a valid FAT12 system.
    fat12_bpb_t bpb = (fat12_bpb_t)device_read_sector(dev, 0);

    // Check to see if it is actually a FAT12 system...
    if (bpb->bytes_per_sector == 0 || fat12_total_clusters(bpb) >= 4085) {
        // This is not a valid FAT file system, so return false.
        free(bpb);
        return 0;
    }

    if (bpb_out) {
        *bpb_out = bpb;
    }

    // At this point we can assume we're FAT12.
    return 1;
}

void *fat12_mount(vfs_t fs)
{
    fat12_bpb_t bpb = NULL;
    if (!fat12_test(fs->device, &bpb)) {
        return NULL;
    }

    fat12_t fat = calloc(1, sizeof(*fat));
    fat->bpb = bpb;

    return fat;
}

void fat12_unmount(vfs_t fs)
{
    if (fs) {
        if (fs->assoc_info) {
            fat12_t fat = (fat12_t)fs->assoc_info;
            free(fat->bpb);
        }
        free(fs->assoc_info);
        fs->assoc_info = NULL;
    }
}


#pragma mark - FAT Formatting

void fat12_copy_padded_string(char *dst,
                              const void *src,
                              uint8_t n,
                              char pad,
                              uint8_t pad_n)
{
    // Fill the destination with the padding characters
    memset(dst, pad, pad_n);

    // Copy the source for n bytes or the source length (which ever is less)
    // into the destination.
    if (src) {
        memcpy(dst, src, n);
    }
}


void fat12_format_device(
    vdevice_t dev, 
    const char *label, 
    uint8_t *bootsector,
    uint8_t *reserved_data,
    uint16_t additional_reserved_sectors
) {
    // FAT12 Constants Required in the boot sector.
    uint8_t jmp[3] = {0xEB, 0x3C, 0x90};
    const char *oem = "MSWIN4.1";
    const char *system_id = "FAT12";

    // Create a new BIOS Parameter Block and populate it.
    fat12_bpb_t bpb = calloc(1, sizeof(*bpb));
    fat12_copy_padded_string((char *)bpb->jmp, jmp, 3, 0x00, 3);
    fat12_copy_padded_string((char *)bpb->oem, oem, 8, ' ', 8);
    fat12_copy_padded_string((char *)bpb->label, label, 11, ' ', 11);
    fat12_copy_padded_string((char *)bpb->system_id, system_id, 8, ' ', 8);

    // If the boot sector has been provided, then carve out the code
    // portion of it.
    if (bootsector) {
        memcpy(bpb->boot_code, bootsector + 62, sizeof(bpb->boot_code));
    }

    bpb->bytes_per_sector = dev->sector_size;
    bpb->sectors_per_cluster = 1;
    bpb->reserved_sectors = 1 + additional_reserved_sectors;
    bpb->table_count = 2;
    bpb->directory_entries = 224;
    bpb->total_sectors_16 = device_total_sectors(dev);
    bpb->media_type = 0xF8;
    bpb->sectors_per_fat = 9; // This is the value for the above type of media.
    bpb->sectors_per_track = 18; // Again the value for the above type of media.
    bpb->heads = 2; // And again...
    bpb->hidden_sectors = 0;
    bpb->total_sectors_32 = 0; // Unused in FAT12
    bpb->drive = (uint8_t)dev->media;
    bpb->nt_reserved = 1;
    bpb->signature = 0x29;
    bpb->volume_id = 77;//arc4random_uniform(0xFFFFFFFF);
    bpb->boot_signature = 0xAA55;

    // We now need to write the boot sector out to the device.
    device_write_sector(dev, 0, (uint8_t *)bpb);

    // If there are reserved sectors, then write those sectors.
    if (additional_reserved_sectors > 0) {
        device_write_sectors(
            dev, 
            1,
            additional_reserved_sectors,
            (uint8_t *)reserved_data
        );
    }

    // And clean up!
    free(bpb);
}


#pragma mark - Short File Names (8.3 Format)

const char *fat12_convert_to_short_name(const char *name, uint8_t tn)
{
    // Setup a buffer for the name. Fill it with spaces to act as padding
    // in the event that we don't fill it entirely.
    char *buffer = calloc(8, sizeof(*buffer));
    memset(buffer, ' ', 8);

    // Perform some calculations to determine exactly what needs to be done.
    uint32_t len = (uint32_t)strlen(name);
    uint32_t cut = len > 8 ? 6 : len;
    uint8_t i = 0;

    // Step through the name and extract it to the the buffer. Once `i` reaches
    // `cut` then insert a `~` and the truncation number.
    while (i < cut) {
        char c = *name++;

        // Certain character's are _not_ allowed. We're going to ignore them.
        int is_valid = (c >= 'A' && c <= 'Z') || (c >= '0' && c <= '9');
        is_valid = is_valid || c == '!' || c == '#' || c == '$' || c == '%';
        is_valid = is_valid || c == '&' || c == '\'' || c == '(' || c == ')';
        is_valid = is_valid || c == '-' || c == '@' || c == '^' || c == '_';
        is_valid = is_valid || c == '`' || c == '{' || c == '}' || c == '~';

        if (!is_valid) {
            // The character is not allowed, but that does not mean it shouldn't
            // enter the buffer as an uppercase or underscore character.
            if (c >= 'a' && c <= 'z') {
                c -= ('a' - 'A');
            }
            else if (c == '+') {
                c = '_';
            }
            else {
                continue;
            }
        }

        // We have a valid character to add to the buffer.
        buffer[i++] = c;
    }

    // If the short name is truncated then we need to correctly terminate it.
    if (len > 8) {
        buffer[6] = '~';
        buffer[7] = ((tn >= 1 && tn <= 9) ? tn : 1) + '0';
    }

    // Return the converted name back to the caller.
    return buffer;
}

const char *fat12_convert_to_extension(const char *extension)
{
    // Setup a buffer for the name. Fill it with spaces to act as padding
    // in the event that we don't fill it entirely.
    char *buffer = calloc(3, sizeof(*buffer));
    memset(buffer, ' ', 3);

    // Perform some calculations to determine what exactly needs to be done.
    uint32_t len = (uint32_t)strlen(extension);
    uint8_t cut = len > 3 ? 3 : len;
    uint8_t i = 0;

    // Step through the extension and extract it to the buffer. Once `i` reaches
    // `cut` then stop. Only uppercase alpha numeric characters are valid here.
    while (i < cut) {
        char c = *extension++;

        // Certain character's are _not_ allowed. We're going to ignore them.
        int is_valid = (c >= 'A' && c <= 'Z') || (c >= '0' && c <= '9');

        if (!is_valid) {
            // The character is not allowed, but that does not mean it shouldn't
            // enter the buffer as an uppercase or underscore character.
            if (c >= 'a' && c <= 'z') {
                c -= ('a' - 'A');
            }
            else {
                continue;
            }
        }

        // We have a valid character to add to the buffer
        buffer[i++] = c;

    }

    // Return the converted extension back to the caller
    return buffer;
}

const char *fat12_construct_short_name(const char *name, uint8_t tn)
{
    // Before trying to construct the short name, we need to parse the name that
    // has been provided and extract both the filename and the extension.
    char *filename = NULL;
    char *extension = NULL;
    vfs_parse_filename(name, &filename, &extension);

    // Now convert each component into the required short name format.
    const char *sfn_filename = fat12_convert_to_short_name(filename, tn);
    const char *sfn_extension = fat12_convert_to_extension(extension);

    // Construct the new short file name.
    char *sfn = calloc(11, sizeof(*sfn));
    memcpy(sfn, sfn_filename, 8);
    memcpy(sfn + 8, sfn_extension, 3);

    // Clean up any memory used.
    free((void *)filename);
    free((void *)extension);
    free((void *)sfn_filename);
    free((void *)sfn_extension);

    // Finally return the result to the caller.
    return sfn;
}


#pragma mark - Standard File Names (Standard Representation)

const char *fat12_construct_standard_name_from_sfn(const char *sfn)
{
    char *name = calloc(13, sizeof(*name));
    char *ptr = name;

    for (uint8_t i = 0; i < 11; ++i) {
        char c = sfn[i];

        // We're at the start of the extension. Add a `.` to the name, unless
        // there is a space in the first slot of the extension. If there is,
        // then break as we've finished the name.
        if (i == 8 && c == ' ') {
            break;
        }
        else if (i == 8) {
            *ptr++ = '.';
        }
        // Spaces should only occur in the padding of the name, so we're going
        // to ignore it.
        else if (c == ' ') {
            continue;
        }

        // After all of that add the character to the name.
        *ptr++ = sfn[i];
    }

    // Finally return the name to the caller.
    return name;
}


#pragma mark - File Allocation Table

void fat12_load_fat_table(vfs_t fs)
{
    assert(fs);
    fat12_t fat = fs->assoc_info;
    if (!fat->fat_data) {
        // get the start of the FAT
        uint32_t fat_start = fat12_fat_start(fat->bpb, 0);
        uint32_t fat_size = fat12_fat_size(fat->bpb, 1);

        // read all the sectors of the FAT into a buffer
        fat->fat_data = device_read_sectors(fs->device, fat_start, fat_size);
    }
}

void fat12_destroy_fat_table(vfs_t fs)
{
    assert(fs);
    fat12_t fat = fs->assoc_info;
    free(fat->fat_data);
    fat->fat_data = NULL;
}

void fat12_flush_fat_table(vfs_t fs)
{
    assert(fs);

    fat12_t fat = fs->assoc_info;

    uint32_t fat_start = fat12_fat_start(fat->bpb, 0);
    uint32_t fat_size = fat12_fat_size(fat->bpb, 1);
    uint32_t fat2_start = fat12_fat_start(fat->bpb, 1);
    uint32_t fat2_size = fat12_fat_size(fat->bpb, 1);

    device_write_sectors(fs->device, fat_start, fat_size, fat->fat_data);
    device_write_sectors(fs->device, fat2_start, fat2_size, fat->fat_data);
}

uint16_t fat12_fat_table_entry(vfs_t fs, uint32_t entry)
{
    assert(fs);

    // If the entry is an invalid one then simply return EOF
    if (entry < 2 || entry == fat12_cluster_ref_eof) {
        return fat12_cluster_ref_eof;
    }

    fat12_t fat = fs->assoc_info;

    // Convert the entry to an absolute offset in the FAT. We need to ensure
    // we're on a multiple of 3 boundary and rounding _down_.
    // Entries are also twinned together in 3 byte groups. Are we looking at
    // the first entry or the second one?
    uint8_t which = entry % 2;
    entry -= (entry % 2);
    uint32_t off = (entry * 3) / 2;

    // Read the entry
    if (which == 0) {
        // first...
        return (fat->fat_data[off] + (fat->fat_data[off+1] << 8)) & 0x0FFF;
    }
    else {
        // second...
        return (fat->fat_data[off+1] + (fat->fat_data[off+2] << 8)) >> 4;
    }
}

void fat12_fat_table_set_entry(vfs_t fs, uint32_t entry, uint16_t value)
{
    assert(fs);

    // If the entry is an invalid one then abort.
    if (entry < 2 || entry == fat12_cluster_ref_eof) {
        return;
    }

    fat12_t fat = fs->assoc_info;

    // Convert the entry to an absolute offset in the FAT. We need to ensure
    // we're on a multiple of 3 boundary and rounding _down_.
    // Entries are also twinned together in 3 byte groups. Are we looking at
    // the first entry or the second one?
    uint8_t which = entry % 2;
    entry -= (entry % 2);
    uint32_t off = (entry * 3) / 2;


    // Write the entry
    if (which == 0) {
        // first...
        fat->fat_data[off] = value & 0xFF;
        fat->fat_data[off+1] = (fat->fat_data[off+1] & 0xF0);
        fat->fat_data[off+1] |= ((value >> 8) & 0x0F);
    }
    else {
        // second...
        fat->fat_data[off+2] = (value >> 4) & 0xff;
        fat->fat_data[off+1] = (fat->fat_data[off+1] & 0x0F);
        fat->fat_data[off+1] |= ((value << 4) & 0xF0);
    }
}


#pragma mark - Clusters

uint32_t fat12_cluster_count_for_size(vfs_t fs, uint32_t n)
{
    assert(fs);
    fat12_t fat = fs->assoc_info;
    fat12_bpb_t bpb = fat->bpb;
    uint32_t sectors = (n + (bpb->bytes_per_sector-1)) / bpb->bytes_per_sector;
    return MAX(sectors / fat->bpb->sectors_per_cluster, 1);
}

uint16_t fat12_first_available_cluster(vfs_t fs)
{
    assert(fs);
    fat12_load_fat_table(fs);
    fat12_t fat = fs->assoc_info;

    // We're going to step through the clusters and determine
    // which is the first available one.
    uint32_t cluster_count = (fat12_fat_size(fat->bpb, 1)
                              * fat->bpb->bytes_per_sector) / 3;
    for (uint16_t i = 2; i < cluster_count; ++i) {
        // Get the cluster value
        uint32_t entry = fat12_fat_table_entry(fs, i);
        if (entry == fat12_cluster_ref_free) {
            return i;
        }
    }

    // At this point we have encountered a serious error!
    // We're just going to throw at this point.
    fprintf(stderr, "No free clusters available!\n");
    exit(1);

    return 0;
}

uint32_t fat12_is_valid_cluster(uint16_t cluster)
{
    return (cluster >= 0x002 && cluster <= 0xFFE);
}

uint32_t fat12_is_available_cluster(uint16_t cluster)
{
    return (cluster == fat12_cluster_ref_free);
}

uint32_t fat12_is_eof_cluster(uint16_t cluster)
{
    return (cluster == fat12_cluster_ref_eof);
}


uint16_t fat12_next_cluster(vfs_t fs, uint16_t cluster)
{
    fat12_load_fat_table(fs);
    
    // Ensure we only have the lower 12 bits of the cluster number.
    // If the cluster is an end of file cluster then return back immediately.
    cluster = (cluster & fat12_cluster_mask);
    if (cluster == fat12_cluster_ref_eof) {
        return cluster;
    }

    // Look up the value of the cluster. That's the next one.
    assert(fs);

    uint16_t entry = fat12_fat_table_entry(fs, cluster);
    return entry;
}

uint32_t fat12_sector_for_cluster(vfs_t fs, uint16_t cluster)
{
    assert(fs);
    fat12_t fat = fs->assoc_info;

    // If the cluster is 0, then treat it as the root directory
    if (cluster == 0) {
        return fat12_root_directory_start(fat->bpb);
    }
    else {
        uint32_t sector = ((cluster - 2) * fat->bpb->sectors_per_cluster);
        sector += fat12_data_start(fat->bpb);
        return sector;
    }
}


#pragma mark - Directories

void fat12_destroy_working_directory(fat12_t fat)
{
    assert(fat);
    vfs_node_destroy(fat->current_dir.first_child);
    fat->current_dir.first_child = NULL;
    fat->current_dir.last_child = NULL;
    fat->current_dir.sfn.first_cluster = 0;
    fat->current_dir.index = 0;
}

uint32_t fat12_directory_starting_cluster(vfs_node_t directory)
{
    if (directory) {
        fat12_sfn_t sfn = directory->assoc_info;
        return sfn->first_cluster;
    }

    return 0;
}

uint32_t fat12_directory_starting_sector(vfs_t fs, vfs_node_t directory)
{
    fat12_t fat = fs->assoc_info;

    if (directory) {
        fat12_sfn_t sfn = directory->assoc_info;
        return fat12_sector_for_cluster(fs, sfn->first_cluster);
    }

    return fat12_root_directory_start(fat->bpb);
}

uint32_t fat12_directory_size(vfs_t fs)
{
    fat12_t fat = fs->assoc_info;
    return fat12_root_directory_size(fat->bpb);
}

enum vfs_node_state fat12_node_state_from_name(uint8_t *name)
{
    if (*name == 0xE5) {
        return vfs_node_available; // Deleted Entry
    }
    else if (*name == 0x00) {
        return vfs_node_unused; // Never used
    }
    else {
        return vfs_node_used; // In use
    }
}

vfs_node_t fat12_construct_node_for_sfn(vfs_t fs, void *dir_data, uint32_t sfni)
{
    assert(fs);
    assert(dir_data);
    
    fat12_t fat = fs->assoc_info;
    fat12_bpb_t bpb = fat->bpb;
    
    // Ensure that we're not asking for an entry beyond the end of the
    // directory.
    assert(sfni < bpb->directory_entries);
    
    // Extract the relavent directory entry.
    uintptr_t ptr = (uintptr_t)dir_data + (sfni * sizeof(struct fat12_sfn));
    
    fat12_sfn_t sfn = calloc(1, sizeof(*sfn));
    memcpy(sfn, (void *)ptr, sizeof(*sfn));
    
    const char *sfn_name = (const char *)sfn->name;
    const char *name = fat12_construct_standard_name_from_sfn(sfn_name);
    uint8_t attributes = fat12_translate_to_vfs_attributes(sfn->attribute);
    enum vfs_node_state state = fat12_node_state_from_name(sfn->name);
    
    // Construct the VFS node and copy out all appropriate entries
    vfs_node_t node = vfs_node_init(fs, name, attributes, state, sfn);
    node->size = sfn->size;
    node->creation_time = fat12_date_time_to_posix(sfn->cdate, sfn->ctime);
    node->modification_time = fat12_date_time_to_posix(sfn->mdate, sfn->mtime);
    node->access_time = fat12_date_time_to_posix(sfn->adate, 0);
    
    // Clean up
    free((void *)name);
    
    // Finally return the node to the caller
    return node;
}

fat12_sfn_t fat12_commit_node_changes_to_sfn(vfs_node_t node)
{
    fat12_sfn_t sfn = node->assoc_info;
    
    if (node->is_dirty) {
        sfn->attribute = fat12_translate_from_vfs_attributes(node->attributes);
        sfn->size = (uint32_t)node->size;
        sfn->ctime = fat12_time_from_posix(node->creation_time);
        sfn->cdate = fat12_date_from_posix(node->creation_time);
        sfn->mtime = fat12_time_from_posix(node->modification_time);
        sfn->mdate = fat12_date_from_posix(node->modification_time);
        sfn->adate = fat12_date_from_posix(node->access_time);
        
        // If the node has been deleted, then just toggle the name bit in the
        // SFN rather than rebuilding the name. This is due to a slight bug
        // in the short name constructor which does same strange mangling of
        // the name...
        if ((uint8_t)node->name[0] == 0xE5) {
            sfn->name[0] = 0xe5;
        }
        else {
            const char *sfn_name = fat12_construct_short_name(node->name, 1);
            fat12_copy_padded_string((void *)sfn->name, sfn_name, 11, ' ', 11);
            free((void *)sfn_name);
        }
        
        node->is_dirty = 0;
    }
    
    return sfn;
}

void fat12_load_directory(vfs_t fs, vfs_node_t directory)
{
    assert(fs);

    fat12_t fat = fs->assoc_info;

    // Read the data from the device for the new directory. Get this now, as
    // we're about to lose it.
    uint32_t cluster = fat12_directory_starting_cluster(directory);
    uint32_t start = fat12_directory_starting_sector(fs, directory);
    uint32_t count = fat12_directory_size(fs);
    
    // Copy out the SFN from the directory to the working directory
    if (directory) {
        memcpy(&fat->current_dir.sfn,
               directory->assoc_info,
               sizeof(struct fat12_sfn));
    }

    // Clean up the previous directory, this will destroy the node for the
    // directory we're about to enter.
    fat12_destroy_working_directory(fat);

    // Read the contents of the directory.
    uint8_t *buffer = device_read_sectors(fs->device, start, count);

    // Begin parsing through nodes and populating them.
    uint32_t entry_count = ((count * fat->bpb->bytes_per_sector) / 32);

    for (uint32_t i = 0; i < entry_count; ++i) {
        // Get the node for the entry number
        vfs_node_t node = fat12_construct_node_for_sfn(fs, buffer, i);

        if (fat->current_dir.last_child) {
            fat->current_dir.last_child->next_sibling = node;
            node->prev_sibling = fat->current_dir.last_child;
        }
        fat->current_dir.last_child = node;

        if (!fat->current_dir.first_child) {
            fat->current_dir.first_child = node;
        }
    }
    
    // Make the directory the working one.
    fat->current_dir.sfn.first_cluster = cluster;

    // Clean up the data
    free(buffer);
}

void fat12_flush_directory(vfs_t fs)
{
    assert(fs);

    fat12_t fat = fs->assoc_info;

    // Setup a buffer for the directory
    uint16_t first_cluster = fat->current_dir.sfn.first_cluster;
    uint32_t sector = fat12_sector_for_cluster(fs, first_cluster);
    uint32_t count = fat12_directory_size(fs);
    uint32_t buffer_size = count * fat->bpb->bytes_per_sector;
    uint8_t *buffer = calloc(buffer_size, sizeof(*buffer));

    // Iterate through the directory nodes and write them back
    // out to the buffer.
    vfs_node_t node = fat->current_dir.first_child;
    uint32_t offset = 0;
    while (node) {
        // Get the SFN back for the node
        fat12_sfn_t sfn = fat12_commit_node_changes_to_sfn(node);

        // Copy the SFN out to the buffer in preparation for flushing the
        // directory.
        memcpy(buffer + offset, sfn, sizeof(struct fat12_sfn));
        offset += sizeof(struct fat12_sfn);
        node = node->next_sibling;
    }

    // Write the sectors out to the device
    device_write_sectors(fs->device, sector, count, buffer);

    // Clean up
    free(buffer);
}

vfs_node_t fat12_current_directory(vfs_t fs)
{
    assert(fs);
    fat12_t fat = fs->assoc_info;
    if (fat->current_dir.sfn.first_cluster == 0) {
        return NULL;
    }
    else {
        return fat12_construct_node_for_sfn(fs, &fat->current_dir.sfn, 0);
    }
}

vfs_node_t fat12_get_directory_list(vfs_t fs)
{
    assert(fs);
    fat12_t fat = fs->assoc_info;
    return fat->current_dir.first_child;
}

void fat12_set_directory(vfs_t fs, vfs_node_t directory)
{
    fat12_load_directory(fs, directory);
}


#pragma mark - Cluster Writing

void fat12_write_cluster_data(vfs_t fs,
                              uint32_t cluster,
                              void *data,
                              uint32_t size)
{
    assert(fs);

    // Get the general fat information
    fat12_t fat = fs->assoc_info;
    fat12_bpb_t bpb = fat->bpb;

    // Create a blank data buffer. This is what we'll write into the cluster.
    // We'll add in the data we've been supplied before however.
    uint32_t buffer_size = bpb->sectors_per_cluster * bpb->bytes_per_sector;
    uint8_t *buffer = calloc(buffer_size, sizeof(*buffer));

    // Copy in the data to the buffer.
    assert(size <= buffer_size);
    memcpy(buffer, data, size);

    // Write the buffer to the cluster
    uint32_t sector = fat12_sector_for_cluster(fs, cluster);
    device_write_sectors(fs->device, sector, bpb->sectors_per_cluster, buffer);

    // Clean up
    free(buffer);
}

uint16_t fat12_reallocate_cluster_chain(vfs_t fs, uint16_t cluster, uint32_t n)
{
    assert(fs);

    // We're going to step through the cluster chain and keep allocating
    // clusters until we exhaust `n`. However there are some complications. If
    // a cluster already exists in the chain, then we do not allocate it if `n`
    // is still greater than 0.  If more clusters exist in the chain and `n` is
    // equal to or less than 0, then we mark the cluster as free. If `n` is 0,
    // then we mark that cluster as end of file (EOF).
    int32_t clusters_remaining = n;
    uint32_t previous_cluster = 0;
    uint32_t start_cluster = fat12_cluster_ref_eof;
    
    while (cluster != fat12_cluster_ref_eof || clusters_remaining >= 0) {
        // Get the original next cluster. We may need to overwrite this value
        // in the course of the checks.
        uint32_t original_next_cluster = fat12_next_cluster(fs, cluster);
        
        // Do we need to allocate another cluster? If we still have clusters
        // that need allocating, and the next cluster is an EOF then yes.
        if (clusters_remaining > 0 && cluster == fat12_cluster_ref_eof) {
            // Acquire a new cluster and mark it appropriately.
            cluster = fat12_first_available_cluster(fs);
            fat12_fat_table_set_entry(fs, cluster, fat12_cluster_ref_eof);
            fat12_fat_table_set_entry(fs, previous_cluster, cluster);
        }

        // Have we reach the final cluster? If so then mark it as EOF.
        else if (clusters_remaining == 0 && cluster != fat12_cluster_ref_eof) {
            // Mark as EOF
            fat12_fat_table_set_entry(fs, cluster, fat12_cluster_ref_eof);
        }

        // Does the chain still contain clusters, but the new length is
        // finished? If so then mark the cluster as free.
        else if (clusters_remaining < 0 && cluster != fat12_cluster_ref_eof) {
            fat12_fat_table_set_entry(fs, cluster, fat12_cluster_ref_free);
        }

        // If we haven't matched any rule, then its probably because nothing
        // needs to be done to the cluster chain. Ignore it.

        // Make sure the active cluster is the next cluster, and decrement the
        // remaining clusters. Also determine if we need to set the starting
        // cluster.
        if (start_cluster == fat12_cluster_ref_eof) {
            start_cluster = cluster;
        }

        previous_cluster = cluster;
        cluster = original_next_cluster;
        --clusters_remaining;
    }

    return start_cluster;
}

uint32_t *fat12_sectors_in_cluster_chain(vfs_t fs,
                                         uint32_t cluster,
                                         uint32_t *count)
{
    assert(fs);
    assert(count);

    fat12_t fat = fs->assoc_info;
    fat12_bpb_t bpb = fat->bpb;

    // Generate a block to hold all the sector numbers
    uint32_t sectors_per_cluster = bpb->sectors_per_cluster;
    uint32_t *sectors = calloc(*count * sectors_per_cluster, sizeof(*sectors));

    // Step through each of the clusters and add there sectors to the list
    uint32_t i = 0;
    while (cluster != fat12_cluster_ref_eof) {
        // Handle the scenario where muliple sectors make up a single cluster.
        uint32_t root_sector = fat12_sector_for_cluster(fs, cluster);
        for (uint8_t j = 0; j < sectors_per_cluster; ++j) {
            sectors[i++] = root_sector + j;
        }

        // Get the next cluster
        cluster = fat12_next_cluster(fs, cluster);
    }

    // Recalculate the count. This should now hold the number of sectors.
    *sectors *= sectors_per_cluster;
    return sectors;
}

void fat12_file_write(vfs_t fs, const char *filename, void *data, uint32_t n)
{
    assert(fs);
    
    fat12_t fat = fs->assoc_info;
    fat12_bpb_t bpb = fat->bpb;
    
    // We first all need to determine how many clusters are going to be needed
    // for the file.
    uint32_t clusters = fat12_cluster_count_for_size(fs, n);
    uint32_t cluster_size = bpb->bytes_per_sector * bpb->sectors_per_cluster;
    
    // We need to search for the get the vfs node for the file. Indicate that it
    // should be made if it does not already exist!
    vfs_node_t node = fat12_get_file(fs, filename, 1, 0);
    if (!node) {
        fprintf(stderr, "Could not write file. File could not be created!\n");
        return;
    }
    
    // Get the actual directory entry for the file as it will contain useful
    // information. Mark the node as dirty so that we actually flush any
    // changes.
    fat12_sfn_t sfn = node->assoc_info;
    node->is_dirty = 1;
    node->size = n;
    vfs_node_update_modification_time(node);
    vfs_node_update_access_time(node);
    
    // We should now reallocate the cluster chain for the file.
    sfn->first_cluster = fat12_reallocate_cluster_chain(fs,
                                                        sfn->first_cluster,
                                                        clusters);

    // Generate a list of sectors for the file.
    node->sector_count = clusters;
    node->sectors = fat12_sectors_in_cluster_chain(fs,
                                                   sfn->first_cluster,
                                                   &node->sector_count);
    
    // We're now ready to begin writing out clusters. We need to work out the
    // size of a cluster in bytes and step through the data buffer, passing it
    // at each offset to the cluster writing function.
    uint32_t cluster = sfn->first_cluster;
    for (uint32_t i = 0; i < clusters; ++i) {
        uint32_t data_offset = i * cluster_size;
        uint32_t data_len = MIN(cluster_size, n - data_offset);
        uintptr_t ptr = (uintptr_t)data + (uintptr_t)data_offset;
        fat12_write_cluster_data(fs, cluster, (void *)ptr, data_len);
        cluster = fat12_next_cluster(fs, cluster);
    }
    
    // Flush the working directory to reflect changes we've made to an entry.
    fat12_flush(fs);
}


#pragma mark - Cluster Reading

void fat12_read_cluster_data(vfs_t fs,
                             uint32_t cluster,
                             void *data,
                             uint32_t size)
{
    assert(fs);
    
    fat12_t fat = fs->assoc_info;
    fat12_bpb_t bpb = fat->bpb;
    
    // Read the cluster in to the buffer
    void *buffer = device_read_sectors(fs->device,
                                       fat12_sector_for_cluster(fs, cluster),
                                       bpb->sectors_per_cluster);
    
    // Transfer to the data supplied
    if (data) {
        memcpy(data, buffer, size);
    }
    
    // Clean up
    free(buffer);
}

uint32_t fat12_file_read(vfs_t fs, const char *name, void **data)
{
    assert(fs);
    assert(data);
    
    // Get the file in question. If we can't find it then ensure data out is
    // NULL and return 0.
    vfs_node_t node = fat12_get_file(fs, name, 0, 0);
    if (!node) {
        *data = NULL;
        return 0;
    }
    
    // Get the directory entry for the file so that we can access cluster
    // information.
    fat12_t fat = fs->assoc_info;
    fat12_bpb_t bpb = fat->bpb;
    fat12_sfn_t sfn = node->assoc_info;
    
    // We now need to allocate enough space for the data to reside.
    *data = calloc(node->size, sizeof(uint8_t));
    
    // Read out data until we have received all the data from the device.
    uint32_t bytes_received = 0;
    uint16_t cluster = sfn->first_cluster;
    uint32_t cluster_size = bpb->bytes_per_sector * bpb->sectors_per_cluster;
    
    while (bytes_received < node->size) {
        uint32_t data_len = MIN(cluster_size, node->size - bytes_received);
        uint32_t offset = bytes_received;
        bytes_received += data_len;
        fat12_read_cluster_data(fs, cluster, (*data) + offset, data_len);
        cluster = fat12_next_cluster(fs, cluster);
    }
    
    return node->size;
}


#pragma mark - Directory Entries (File Support)

fat12_sfn_t fat12_dir_entry_new(vfs_t fs,
                                const char *filename,
                                uint32_t size,
                                uint8_t attributes)
{
    assert(fs);
    
    fat12_t fat = fs->assoc_info;
    
    // Before starting construction of the directory entry, calculate how many
    // clusters are going to be required by the file. This value must be at
    // least one.
    uint32_t clusters = (size / fat->bpb->bytes_per_sector);
    clusters /= fat->bpb->sectors_per_cluster;
    clusters = clusters > 0 ? clusters : 1;
    
    // Go ahead an acquire the cluster chain up front for the file.
    uint16_t cluster = fat12_reallocate_cluster_chain(fs,
                                                      fat12_cluster_ref_eof,
                                                      clusters);
    
    // Calculate the short filename for the provided filename. This should
    // check all entries in the current filename for duplicates and increase
    // the truncation number.
    // TODO: Fix this at somepoint.
    const char *sfn_name = fat12_construct_short_name(filename, 1);
    
    // Begin constructing the directory entry.
    fat12_sfn_t sfn = calloc(1, sizeof(*sfn));
    fat12_copy_padded_string((char *)sfn->name, sfn_name, 11, ' ', 11);
    free((void *)sfn_name);
    sfn->attribute = attributes;
    sfn->first_cluster = cluster;
    sfn->size = size;
    
    // Set the creation time.
    time_t now = time(NULL);
    sfn->cdate = fat12_date_from_posix(now);
    sfn->mdate = sfn->cdate;
    sfn->adate = sfn->adate;
    
    sfn->ctime = fat12_time_from_posix(now);
    sfn->mtime = sfn->ctime;
    
    // Return the directory entry to the caller
    return sfn;
}

void fat12_create_file_node(vfs_node_t node,
                            const char *filename,
                            uint32_t size,
                            enum vfs_node_attributes attributes)
{
    assert(node);
    
    // Convert the attributes into something that the FAT12 file system will
    // understand
    uint8_t fat_attr = fat12_translate_from_vfs_attributes(attributes);

    // Construct the directory entry first, add it to the node and mark it dirty
    fat12_sfn_t sfn = fat12_dir_entry_new(node->fs, filename, size, fat_attr);
    node->assoc_info = sfn;
    node->is_dirty = 1;
    node->size = size;
    node->state = vfs_node_used;
    node->attributes = attributes;
    node->creation_time = fat12_date_time_to_posix(sfn->cdate, sfn->ctime);
    node->modification_time = fat12_date_time_to_posix(sfn->mdate, sfn->mtime);
    node->access_time = fat12_date_time_to_posix(sfn->adate, 0);

    // Generate a list of sectors for the file.
    node->sector_count = fat12_cluster_count_for_size(node->fs, size);
    node->sectors = fat12_sectors_in_cluster_chain(node->fs,
                                                   sfn->first_cluster,
                                                   &node->sector_count);
    
    // The final task is to extract the regular filename from the SFN.
    free((void *)node->name);
    const char *sfn_name = (const char *)sfn->name;
    node->name = fat12_construct_standard_name_from_sfn(sfn_name);
}

void fat12_create_directory_node(vfs_node_t node,
                                 const char *filename,
                                 enum vfs_node_attributes attributes)
{
    assert(node);

    fat12_t fat = node->fs->assoc_info;
    fat12_bpb_t bpb = fat->bpb;
    
    // Add in the directory attribute.
    attributes |= vfs_node_directory_attribute;
    
    // Calculate in bytes how much space is required for a directory.
    uint32_t size = ((fat12_root_directory_size(fat->bpb)
                      * fat->bpb->sectors_per_cluster)
                     * fat->bpb->bytes_per_sector);
    
    // Construct the actual node for the directory.
    fat12_create_file_node(node, filename, size, attributes);
    fat12_sfn_t sfn = node->assoc_info;
    node->size = 0;
    node->is_dirty = 1;
    
    // Create a new blank sector for the directory. We need to populate two
    // entries into it. These entries are `.` and `..` which are required for
    // a directory.
    uint32_t data_len = bpb->sectors_per_cluster * bpb->bytes_per_sector;
    uint8_t *data = calloc(data_len, sizeof(*data));
    fat12_sfn_t entries = (fat12_sfn_t)data;
    
    // First entry is `.`
    fat12_copy_padded_string((char *)entries[0].name, ".", 1, ' ', 11);
    entries[0].first_cluster = sfn->first_cluster;
    entries[0].attribute = fat12_attribute_directory;
    
    // Second entry is `..`
    fat12_copy_padded_string((char *)entries[1].name, "..", 2, ' ', 11);
    entries[1].first_cluster = 0x000;
    entries[1].attribute = fat12_attribute_directory;

    // Finally write directory data out to the first cluster
    fat12_write_cluster_data(node->fs, sfn->first_cluster, data, data_len);

    // Clean up
    free(data);
}

uint8_t fat12_is_node_available(vfs_node_t node)
{
    return node->state == vfs_node_unused || node->state == vfs_node_available;
}


#pragma mark - High Level File Support

vfs_node_t fat12_get_node(vfs_t fs, const char *name)
{
    return fat12_get_file(fs, name, 0, 0);
}

vfs_node_t fat12_get_file(vfs_t fs,
                          const char *name,
                          uint8_t create_missing,
                          uint8_t creation_attributes)
{
    // Convert the name to a name that is understood on the FAT file system.
    const char *sfn = fat12_construct_short_name(name, 1);
    const char *reg = fat12_construct_standard_name_from_sfn(sfn);
    
    // Get the working directory and scan through the files for a match.
    fat12_t fat = fs->assoc_info;
    vfs_node_t node = fat->current_dir.first_child;
    vfs_node_t first_available = NULL;
    while (node) {
        
        // Check if the node actually exists. If it doesn't then assume the
        // directory has no more entries.
        if (node->state == vfs_node_unused) {
            first_available = first_available ?: node;
            break;
        }
        
        // Check if the node exists, but is actually a deleted entry. If it is
        // a deleted entry, then check if it is the first available.
        else if (node->state == vfs_node_available) {
            first_available = first_available ?: node;
        }
        
        // Everything else should be tested as if it were a normal entry. Test
        // the name.
        else if (strcmp(node->name, reg) == 0 ||
                 strcmp(node->name, name) == 0)
        {
            // The node is a metch. This is the file we were looking for.
            break;
        }
        
        // Nothing matched or significant happened... move on to the next node.
        node = node->next_sibling;
    }
    
    // Should we attempt to create a node for the file, if no file existed?
    if (first_available && create_missing) {
        node = first_available;
        
        if (creation_attributes & vfs_node_directory_attribute) {
            fat12_create_directory_node(node, name, creation_attributes);
        }
        else {
            fat12_create_file_node(node, name, 0, creation_attributes);
        }
        
        node = first_available;
    }
    
    // Clean up
    free((void *)sfn);
    free((void *)reg);
    
    // Flush the working directory to reflect changes
    fat12_flush(fs);
    
    // Return the node to the caller
    return node;
}

void fat12_create_file(vfs_t fs, const char *name, enum vfs_node_attributes a)
{
    fat12_get_file(fs, name, 1, a);
}

vfs_node_t fat12_create_dir(vfs_t fs,
                            const char *name,
                            enum vfs_node_attributes a)
{
    return fat12_get_file(fs, name, 1, a | vfs_node_directory_attribute);
}

void fat12_remove_file(vfs_t fs, const char *name)
{
    // Find the file that needs to be removed.
    vfs_node_t node = fat12_get_file(fs, name, 0, 0);
    if (!node) {
        return;
    }
    
    // Mark the first character of the name as 0xE5 to indicate it's been
    // deleted.
    *((uint8_t *)node->name) = 0xe5;
    node->is_dirty = 1;
    node->state = vfs_node_available;
    
    // We also need to destroy the cluster chain and mark everything as
    // available.
    fat12_sfn_t sfn = node->assoc_info;
    sfn->first_cluster = fat12_reallocate_cluster_chain(fs,
                                                        sfn->first_cluster,
                                                        0);
    
    // Check the first cluster. If it is not an EOF, then look up the cluster,
    // and set it to be free, and mark the first cluster as EOF.
    if (sfn->first_cluster != fat12_cluster_ref_eof) {
        fat12_fat_table_set_entry(fs,
                                  sfn->first_cluster,
                                  fat12_cluster_ref_free);
        sfn->first_cluster = fat12_cluster_ref_eof;
    }
    
    // Finally force the contents of the directory to be flushed to the device.
    fat12_flush(fs);
}


#pragma mark - Metadata Flushing

void fat12_flush(vfs_t fs)
{
    fat12_flush_fat_table(fs);
    fat12_flush_directory(fs);
}
