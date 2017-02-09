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

#include <fat/fat12.h>
#include <fat/fat12-structures.h>

#include <vfs/vfs.h>
#include <vfs/directory.h>
#include <vfs/node.h>
#include <vfs/path.h>


#ifdef MAX
#   undef MAX
#endif

#define MAX(a,b) \
    ({ __typeof__ (a) _a = (a); \
       __typeof__ (b) _b = (b); \
       _a > _b ? _a : _b; })


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
};


#pragma mark - VFS Interface (Prototypes)

const char *fat12_name();

void *fat12_mount(vfs_t fs);
void fat12_unmount(vfs_t fs);

void fat12_format_device(vdevice_t dev, const char *label, uint8_t *bootcode);

void fat12_set_directory(vfs_t fs, vfs_directory_t dir);
void *fat12_list_directory(vfs_t fs);

void fat12_file_write(vfs_t fs, const char *name, uint8_t *data, uint32_t n);

void fat12_create_file(vfs_t, const char *, enum vfs_node_attributes);
void fat12_create_dir(vfs_t fs, const char *name, enum vfs_node_attributes a);


#pragma mark - VFS Interface Creation

vfs_interface_t fat12_init()
{
    vfs_interface_t fs = vfs_interface_init();

    fs->type_name = fat12_name;

    fs->mount_filesystem = fat12_mount;
    fs->unmount_filesystem = fat12_unmount;

    fs->format_device = fat12_format_device;

    fs->set_directory = fat12_set_directory;
    fs->list_directory = fat12_list_directory;

    fs->write = fat12_file_write;

    fs->create_file = fat12_create_file;
    fs->create_dir = fat12_create_dir;

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

void fat12_copy_padded_string(char *dst, const void *src, char pad, uint8_t n)
{
    // Fill the destination with the padding characters
    memset(dst, pad, n);

    // Copy the source for n bytes or the source length (which ever is less)
    // into the destination.
    if (src) {
        uint8_t sn = (uint8_t)strlen(src);
        memcpy(dst, src, sn > n ? n : sn);
    }
}


void fat12_format_device(vdevice_t dev, const char *label, uint8_t *bootcode)
{
    // FAT12 Constants Required in the boot sector.
    uint8_t jmp[3] = {0xEB, 0x3C, 0x90};
    const char *oem = "MSWIN4.1";

    // Create a new BIOS Parameter Block and populate it.
    fat12_bpb_t bpb = calloc(1, sizeof(*bpb));
    fat12_copy_padded_string((char *)bpb->jmp, jmp, 0x00, 3);
    fat12_copy_padded_string((char *)bpb->oem, oem, ' ', 8);
    fat12_copy_padded_string((char *)bpb->label, label, ' ', 11);

    if (bootcode) {
        memcpy(bpb->boot_code, bootcode, sizeof(bpb->boot_code));
    }

    bpb->bytes_per_sector = dev->sector_size;
    bpb->sectors_per_cluster = 1;
    bpb->reserved_sectors = 1;
    bpb->table_count = 2;
    bpb->directory_entries = 224;
    bpb->total_sectors_16 = device_total_sectors(dev);
    bpb->media_type = 0xF0; // 3.5-inch, 2-sided 9-sector
    bpb->sectors_per_fat = 9; // This is the value for the above type of media.
    bpb->sectors_per_track = 18; // Again the value for the above type of media.
    bpb->heads = 2; // And again...
    bpb->hidden_sectors = 0;
    bpb->total_sectors_32 = 0; // Unused in FAT12
    bpb->drive = 0;
    bpb->nt_reserved = 0;
    bpb->signature = 0x28;
    bpb->volume_id = arc4random_uniform(0xFFFFFFFF);
    bpb->boot_signature = 0xAA55;

    // We now need to write the boot sector out to the device.
    device_write_sector(dev, 0, (uint8_t *)bpb);

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
    char *sfn = calloc(1, sizeof(*sfn));
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
        fat->fat_data[off+2] = value & 0xff;
        fat->fat_data[off+1] = (fat->fat_data[off+1] & 0x0F);
        fat->fat_data[off+1] |= ((value >> 4) & 0xF0);
    }
}


#pragma mark - Clusters

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
    // Ensure we only have the lower 12 bits of the cluster number.
    // If the cluster is an end of file cluster then return back immediately.
    cluster = (cluster & fat12_cluster_ref_eof);
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
    uint32_t sector = ((cluster - 2) * fat->bpb->sectors_per_cluster);
    sector += fat12_data_start(fat->bpb);
    return sector;
}


#pragma mark - Directories

void fat12_current_directory(vfs_t fs, uint32_t *first_sector, uint32_t *count)
{
    assert(fs);
    assert(first_sector);
    assert(count);

    fat12_t fat = fs->assoc_info;

    // TODO: proper implementation here...
    *first_sector = fat12_root_directory_start(fat->bpb);
    *count = fat12_root_directory_size(fat->bpb);
}

void fat12_destroy_working_directory(fat12_t fat)
{
    assert(fat);
    vfs_directory_destroy(fat->working_directory);
    fat->working_directory = NULL;
}

fat12_directory_info_t fat12_directory_info_make(uint32_t first_sector,
                                                 uint32_t count)
{
    fat12_directory_info_t info = calloc(1, sizeof(*info));
    info->starting_sector = first_sector;
    info->sector_count = count;
    return info;
}

void fat12_load_directory(vfs_t fs, uint32_t first_sector, uint32_t count)
{
    assert(fs);

    fat12_t fat = fs->assoc_info;

    // Clean up the previous directory
    fat12_destroy_working_directory(fat);

    // Read the data from the device for the directory
    uint8_t *buffer = device_read_sectors(fs->device, first_sector, count);

    // Create the working directory and prepare to populate it with nodes.
    fat12_directory_info_t dir_info = fat12_directory_info_make(first_sector,
                                                                count);
    fat->working_directory = vfs_directory_init(fs, NULL, dir_info);

    // Begin parsing through nodes and populating them.
    // TODO: Consider LFN nodes
    uint32_t entry_count = ((count * fat->bpb->bytes_per_sector) / 32);
    for (uint32_t i = 0; i < entry_count; ++i) {

        // Extract the information into the structure
        fat12_sfn_t raw_sfn = (fat12_sfn_t)(buffer + (i * sizeof(*raw_sfn)));
        fat12_sfn_t sfn = calloc(1, sizeof(*sfn));
        memcpy(sfn, raw_sfn, sizeof(*sfn));

        // Construct a new node from the SFN.
        vfs_node_t node = vfs_node_init(fs, sfn);
        node->is_directory = sfn->attribute & fat12_attribute_directory;
        node->is_system = sfn->attribute & fat12_attribute_system;
        node->is_hidden = sfn->attribute & fat12_attribute_hidden;
        node->is_readonly = sfn->attribute & fat12_attribute_readonly;

        // Determine the state of the node.
        if (raw_sfn->name[0] == 0xE5) {
            node->state = vfs_node_available;
        }
        else if (raw_sfn->name[0] == 0x00) {
            node->state = vfs_node_unused;
        }
        else {
            node->state = vfs_node_used;
        }

        // Extract a proper name out of the SFN "TEST    TXT" => "TEST.TXT"
        const char *sfn_name = (const char *)sfn->name;
        node->name = fat12_construct_standard_name_from_sfn(sfn_name);

        // Add the node to the directory
        vfs_directory_t dir = fat->working_directory;
        dir->first = vfs_node_append_node(dir->first, node);
    }

    // Clean up the data
    free(buffer);
}

void fat12_flush_directory(vfs_t fs)
{
    assert(fs);

    fat12_t fat = fs->assoc_info;

    // Setup a buffer for the directory
    fat12_directory_info_t dir_info = fat->working_directory->assoc_info;
    uint32_t buffer_size = dir_info->sector_count * fat->bpb->bytes_per_sector;
    uint8_t *buffer = calloc(buffer_size, sizeof(*buffer));

    // Iterate through the directory nodes and write them back
    // out to the buffer.
    vfs_node_t node = fat->working_directory->first;
    uint32_t offset = 0;
    while (node) {

        // If the node is dirty then we need to reform the SFN.
        if (node->is_dirty) {
            // TODO: Form the name correctly
            fat12_sfn_t sfn = node->assoc_info;

            // Set the attributes of the file accordingly
            sfn->attribute |= (node->is_directory ? 0x10 : 0x00);

            // Mark the node as no longer dirty.
            node->is_dirty = 0;
        }

        memcpy(buffer + offset, node->assoc_info, sizeof(struct fat12_sfn));
        offset += sizeof(struct fat12_sfn);
        node = node->next;
    }

    // Write the sectors out to the device
    device_write_sectors(fs->device,
                         dir_info->starting_sector,
                         dir_info->sector_count,
                         buffer);

    // Clean up
    free(buffer);
}

void fat12_set_directory(vfs_t fs, vfs_directory_t dir)
{
    assert(fs);
    
    uint32_t start = 0;
    uint32_t count = 0;

    if (dir && dir->assoc_info) {
        fat12_directory_info_t info = dir->assoc_info;
        start = info->starting_sector;
        count = info->sector_count;
    }
    else {
        fat12_current_directory(fs, &start, &count);
    }

    fat12_load_directory(fs, start, count);
}

void *fat12_list_directory(vfs_t fs)
{
    assert(fs);
    fat12_t fat = fs->assoc_info;
    
    if (!fat->working_directory) {
        fat12_set_directory(fs, NULL);
    }
    
    return fat->working_directory;
}

vfs_node_t fat12_find_file(fat12_t fat, const char *name)
{
    assert(fat);

    vfs_node_t node = fat->working_directory->first;
    while (node) {
        // Is this the file in question?
        if (strcmp(name, node->name) == 0) {
            break;
        }
        node = node->next;
    }

    return node;
}


#pragma mark - Cluster Writing

void fat12_file_write(vfs_t fs, const char *name, uint8_t *data, uint32_t n)
{
    assert(fs);

    // Search for the file in the current directory. A file should be already
    // present for us to write to. If not, fail the operation.
    fat12_t fat = fs->assoc_info;
    vfs_node_t node = fat12_find_file(fat, name);

    // Is the node NULL? If it is then we failed.
    if (!node) {
        fprintf(stderr, "File \"%s\" does not exist!\n", name);
        return;
    }

    // Look up some required information about it. This may require an
    // adjustment to the cluster chain.
    fat12_sfn_t sfn = node->assoc_info;

    // Has the size changed? If so, how?
    uint32_t old_sectors = (((sfn->size?:1) + (fat->bpb->bytes_per_sector - 1))
                            / fat->bpb->bytes_per_sector);
    uint32_t new_sectors = ((n + (fat->bpb->bytes_per_sector - 1))
                            / fat->bpb->bytes_per_sector);

    // We need to iterate through the cluster chain and write out each of the
    // sectors. When we have exhausted the sectors in the data, if we still have
    // clusters remaining in the old chain, continue traversing it and clearing
    // them. If we run out of old clusters then we need to begin claiming more
    // clusters.
    uint16_t cluster = sfn->first_cluster;
    uint32_t chain_i = 0;
    uint32_t old_clusters = old_sectors * fat->bpb->sectors_per_cluster;
    uint32_t new_clusters = new_sectors * fat->bpb->sectors_per_cluster;
    uint32_t total_clusters = MAX(old_clusters, new_clusters);

    while (chain_i < total_clusters) {

        // The first check is to see if we're outside of the original
        // cluster chain. We need to start claiming clusters in this scenario.
        if (chain_i >= old_clusters) {
            // we need to claim another cluster. Make it the end of file
            // cluster.
            uint32_t new_cluster = fat12_first_available_cluster(fs);
            fat12_fat_table_set_entry(fs, cluster, new_cluster);
            fat12_fat_table_set_entry(fs, new_cluster, fat12_cluster_ref_eof);
        }

        // The next check is to see if we are outside of the new cluster chain
        else if (chain_i >= new_clusters) {
            uint32_t tmp = fat12_next_cluster(fs, cluster);

            if (chain_i == new_clusters - 1) {
                fat12_fat_table_set_entry(fs, cluster, fat12_cluster_ref_eof);
            }
            else {
                fat12_fat_table_set_entry(fs, cluster, fat12_cluster_ref_free);
            }
            cluster = tmp;
            chain_i++;
            continue;
        }

        // Write out the data for this cluster
        uint32_t sector = fat12_sector_for_cluster(fs, cluster);
        uint32_t count = fat->bpb->sectors_per_cluster;
        uint32_t offset = (chain_i * fat->bpb->sectors_per_cluster
                           * fat->bpb->bytes_per_sector);

        // Copy the data into an appropriate buffer first
        uint8_t *buffer = calloc(count, fat->bpb->bytes_per_sector);
        memcpy(buffer, data + offset, n);
        device_write_sectors(fs->device, sector, count, buffer);
        free(buffer);

        // Move to the next cluster in the chain.
        chain_i++;
        cluster = fat12_next_cluster(fs, cluster);
    }

    // Update the metadata
    sfn->size = n;
    node->size = n;

    // Flush changes
    fat12_flush_fat_table(fs);
    fat12_flush_directory(fs);
}

void fat12_clear_cluster(vfs_t fs, uint32_t cluster)
{
    assert(fs);

    fat12_t fat = fs->assoc_info;

    // Create a blank cluster that can be written to the device.
    uint8_t *data = calloc(fat->bpb->sectors_per_cluster,
                           fat->bpb->bytes_per_sector);

    // Write out to the cluster
    uint32_t first_sector = fat12_sector_for_cluster(fs, cluster);
    uint32_t sector_count = fat->bpb->sectors_per_cluster;

    device_write_sectors(fs->device, first_sector, sector_count, data);

    // Clean up
    free(data);
}

uint16_t fat12_acquire_blank_cluster_chain(vfs_t fs, uint32_t n)
{
    assert(fs);
    assert(n > 0);

    // Acquire the first cluster node, and decrement the amount.
    uint16_t first_cluster = fat12_first_available_cluster(fs);
    fat12_clear_cluster(fs, first_cluster);
    fat12_fat_table_set_entry(fs, first_cluster, fat12_cluster_ref_eof);
    --n;

    uint16_t last_cluster = first_cluster;
    while (n--) {
        uint16_t cluster = fat12_first_available_cluster(fs);
        fat12_clear_cluster(fs, cluster);
        fat12_fat_table_set_entry(fs, last_cluster, cluster);
        fat12_fat_table_set_entry(fs, cluster, fat12_cluster_ref_eof);
        last_cluster = cluster;
    }
    
    return first_cluster;
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
    uint16_t first_cluster = fat12_acquire_blank_cluster_chain(fs, clusters);
    
    // Calculate the short filename for the provided filename. This should
    // check all entries in the current filename for duplicates and increase
    // the truncation number.
    // TODO: Fix this at somepoint.
    const char *sfn_name = fat12_construct_short_name(filename, 1);
    
    // Begin constructing the directory entry.
    fat12_sfn_t sfn = calloc(1, sizeof(*sfn));
    fat12_copy_padded_string((char *)sfn->name, sfn_name, ' ', 11);
    sfn->attribute = attributes;
    sfn->first_cluster = first_cluster;
    
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
    
    // Set some convieniece flags
    node->is_directory = attributes & vfs_node_directory_attribute ? 1 : 0;
    node->is_hidden = attributes & vfs_node_hidden_attribute ? 1 : 0;
    node->is_readonly = attributes & vfs_node_read_only_attribute ? 1 : 0;
    node->is_system = attributes & vfs_node_system_attribute ? 1 : 0;
    
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
    
    // Add in the directory attribute.
    attributes |= vfs_node_directory_attribute;
    
    // Calculate in bytes how much space is required for a directory.
    uint32_t size = ((fat12_root_directory_size(fat->bpb)
                      * fat->bpb->sectors_per_cluster)
                     * fat->bpb->bytes_per_sector);
    
    // Construct the actual node for the directory.
    fat12_create_file_node(node, filename, size, attributes);
    fat12_sfn_t sfn = node->assoc_info;
    node->size = sfn->size = 0;
    
    // Create a new blank sector for the directory. We need to populate two
    // entries into it. These entries are `.` and `..` which are required for
    // a directory.
    uint8_t *data = calloc(fat->bpb->sectors_per_cluster,
                           fat->bpb->bytes_per_sector);
    fat12_sfn_t entries = (fat12_sfn_t)data;
    
    // First entry is `.`
    fat12_copy_padded_string((char *)entries[0].name, ".", ' ', 11);
    entries[0].first_cluster = sfn->first_cluster;
    entries[0].attribute = fat12_attribute_directory;
    
    // Second entry is `..`
    fat12_copy_padded_string((char *)entries[1].name, "..", ' ', 11);
    entries[1].first_cluster = 0x000;
    entries[1].attribute = fat12_attribute_directory;

    // Finally write directory data out to the first clust
    uint32_t first_sector = fat12_sector_for_cluster(node->fs,
                                                     sfn->first_cluster);
    uint32_t sector_count = fat->bpb->sectors_per_cluster;

    device_write_sectors(node->fs->device, first_sector, sector_count, data);

    // Clean up
    free(data);
}

uint8_t fat12_is_node_available(vfs_node_t node)
{
    return node->state == vfs_node_unused || node->state == vfs_node_available;
}


#pragma mark - High Level File Support

void fat12_create_file(vfs_t fs, const char *name, enum vfs_node_attributes a)
{
    assert(fs);
    assert(name);

    fat12_t fat = fs->assoc_info;

    // Find the first available entry in the root directory.
    vfs_node_t node = fat->working_directory->first;
    while (node) {
        // If the node is available then touch this one.
        if (fat12_is_node_available(node)) {
            fat12_create_file_node(node, name, 0, a);
            break;
        }
        node = node->next;
    }

    // Flush the directory
    fat12_flush_fat_table(fs);
    fat12_flush_directory(fs);
}

void fat12_create_dir(vfs_t fs, const char *name, enum vfs_node_attributes a)
{
    assert(fs);
    assert(name);

    fat12_t fat = fs->assoc_info;

    // Find the first available entry in the root directory.
    vfs_node_t node = fat->working_directory->first;
    while (node) {
        // If the node is available then touch this one.
        if (fat12_is_node_available(node)) {
            fat12_create_directory_node(node, name, a);
            break;
        }
        node = node->next;
    }

    // Flush the directory
    fat12_flush_fat_table(fs);
    fat12_flush_directory(fs);
}

