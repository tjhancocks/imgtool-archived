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

#define FAT12_ARCHIVE   0x20
#define FAT12_DIRECTORY 0x10
#define FAT12_VOLUMEID  0x08
#define FAT12_SYSTEM    0x04
#define FAT12_HIDDEN    0x02
#define FAT12_READONLY  0x01

#define FAT12_EOF       0xFFF
#define FAT12_FREE      0x000


// prototypes...
const char *fat12_type_name();
void fat12_format_device(vfs_t fs, const char *name, uint8_t *bootcode);
void *fat12_mount(vfs_t fs);
void fat12_unmount(vfs_t fs);
void fat12_touch(vfs_t fs, const char *name);
void fat12_make_directory(vfs_t fs, const char *name);
void fat12_change_directory(vfs_t fs, void *);
void *fat12_list_directory(vfs_t fs);
void fat12_file_write(vfs_t fs, const char *name, uint8_t *data, uint32_t n);


vfs_interface_t fat12_init()
{
    vfs_interface_t fat12 = vfs_interface_init();
    fat12->type_name = fat12_type_name;
    fat12->format_device = fat12_format_device;
    fat12->mount_filesystem = fat12_mount;
    fat12->unmount_filesystem = fat12_unmount;
    fat12->touch = fat12_touch;
    fat12->change_directory = fat12_change_directory;
    fat12->list_directory = fat12_list_directory;
    fat12->write = fat12_file_write;
    fat12->mkdir = fat12_make_directory;
    return fat12;
}


// prototype implementations
const char *fat12_type_name()
{
    return "FAT12";
}

void *fat12_mount(vfs_t fs)
{
    // Load in the bootsector as this contains the bulk of the information that
    // we will need.
    fat12_bpb_t bpb = (fat12_bpb_t)device_read_sector(fs->device, 0);

    // Check to see if it is actually a FAT12 system...
    if (bpb->bytes_per_sector == 0) {
        // This is not a valid FAT file system, so return NULL.
        free(bpb);
        return NULL;
    }

    uint32_t fat_size = bpb->sectors_per_fat;
    uint32_t root_dir_sectors = (((bpb->directory_entries * 32)
                                 + (bpb->bytes_per_sector - 1))
                                 / bpb->bytes_per_sector);
    uint32_t data_sectors = (bpb->total_sectors_16
                             - (bpb->reserved_sectors
                                + (bpb->table_count * fat_size)
                             + root_dir_sectors));
    uint32_t total_clusters = data_sectors / bpb->sectors_per_cluster;

    if (total_clusters >= 4085) {
        // This is not a valid FAT file system, so return NULL.
        free(bpb);
        return NULL;
    }

    // Now setup the fat12 structure for this driver.
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


#pragma mark - Formatting

void fat12_format_device(vfs_t fs, const char *label, uint8_t *bootcode)
{
    // Establish some constants that can be written to the BPB
    uint8_t jmp[3] = {0xEB, 0x3C, 0x90};
    const char oem[8] = {'M','S','W','I','N','4','.','1'};

    // Create a new BPB and populate it.
    fat12_bpb_t bpb = calloc(1, sizeof(*bpb));
    memcpy(bpb->oem, oem, 8);
    memcpy(bpb->label, label, 11);
    memcpy(bpb->jmp, jmp, 3);

    if (bootcode) {
        memcpy(bpb->boot_code, bootcode, sizeof(bpb->boot_code));
    }

    bpb->bytes_per_sector = fs->device->sector_size;
    bpb->sectors_per_cluster = 1;
    bpb->reserved_sectors = 1;
    bpb->table_count = 2;
    bpb->directory_entries = 224;
    bpb->total_sectors_16 = device_total_sectors(fs->device);
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
    device_write_sector(fs->device, 0, (uint8_t *)bpb);

    // And clean up!
    free(bpb);
}


#pragma mark - Calculations

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


#pragma mark - File Names

const char *fat12_construct_short_name(const char *name, int is_dir)
{
    char *short_name = calloc(11, sizeof(*short_name));

    // This is an 8.3 format. That means we need to ensure the extension is
    // 3 characters and the main body is 8 characters. Either truncation or
    // padding needs to occur.
    char *file = NULL;
    char *ext = NULL;
    vfs_parse_filename(name, &file, &ext);

    // Determine what needs to be done with the main body.
    if (is_dir) {
        uint32_t file_len = (uint32_t)strlen(file);
        if (file_len > 11) {
            memcpy(short_name, file, 10);
            short_name[10] = '~';
        }
        else if (file_len < 11) {
            memset(short_name, ' ', 11);
            memcpy(short_name, file, file_len);
        }
        else {
            memcpy(short_name, file, file_len);
        }
    }
    else {
        uint32_t file_len = (uint32_t)strlen(file);
        if (file_len > 8) {
            memcpy(short_name, file, 7);
            short_name[7] = '~';
        }
        else if (file_len < 8) {
            memset(short_name, ' ', 8);
            memcpy(short_name, file, file_len);
        }
        else {
            memcpy(short_name, file, file_len);
        }

        // Handle the extension. This should be 3 characters long. If it isn't
        // throw an exception for now.
        if (strlen(ext) != 3) {
            fprintf(stderr, "File extensions must be 3 characters long!\n");
            exit(1);
        }
        memcpy(short_name + 8, ext, 3);
    }

    // Uppercase everything
    for (uint8_t i = 0; i < 11; ++i) {
        if (short_name[i] >= 'a' && short_name[i] <= 'z') {
            short_name[i] -= ('a' - 'A');
        }
    }

    // Clean up
    free(file);
    free(ext);

    // Return the short name back to the caller.
    return short_name;
}

const char *fat12_construct_standard_name(const char *short_name)
{
    char *name = calloc(13, sizeof(*name));
    char *ptr =  name;

    for (uint8_t i = 0; i < 11; ++i) {

        // We've hit the extension. Add the '.' and ensure we've removed any
        // extraneous spaces.
        if (i == 8) {
            while (*(ptr - 1) == ' ') {
                ptr--;
                *ptr = '\0';
            }
            *ptr++ = '.';
        }

        // Add the next character
        *ptr++ = short_name[i];

    }

    return name;
}


#pragma mark - Current Directory

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
    fat->working_directory = vfs_directory_init(fs, dir_info);

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
        node->is_directory = sfn->attribute & FAT12_DIRECTORY;
        node->is_system = sfn->attribute & FAT12_SYSTEM;
        node->is_hidden = sfn->attribute & FAT12_HIDDEN;
        node->is_readonly = sfn->attribute & FAT12_READONLY;

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
        node->name = fat12_construct_standard_name((const char *)sfn->name);

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

void fat12_change_directory(vfs_t fs, void *path)
{
    assert(fs);

    uint32_t start = 0;
    uint32_t count = 0;

    if (!path) {
        fat12_current_directory(fs, &start, &count);
    }

    fat12_load_directory(fs, start, count);
}

void *fat12_list_directory(vfs_t fs)
{
    assert(fs);
    fat12_t fat = fs->assoc_info;

    if (!fat->working_directory) {
        fat12_change_directory(fs, NULL);
    }

    return fat->working_directory;
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
        if (entry == FAT12_FREE) {
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
    return (cluster == 0x000);
}

uint32_t fat12_is_eof_cluster(uint16_t cluster)
{
    return (cluster == 0xFFF);
}


uint16_t fat12_next_cluster(vfs_t fs, uint16_t cluster)
{
    // Ensure we only have the lower 12 bits of the cluster number.
    // If the cluster is an end of file cluster then return back immediately.
    cluster = (cluster & 0xFFF);
    if (cluster == 0xFFF) {
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


#pragma mark - File Look Up

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


#pragma mark - File Writing

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
            fat12_fat_table_set_entry(fs, new_cluster, FAT12_EOF);
        }

        // The next check is to see if we are outside of the new cluster chain
        else if (chain_i >= new_clusters) {
            uint32_t tmp = fat12_next_cluster(fs, cluster);

            if (chain_i == new_clusters - 1) {
                fat12_fat_table_set_entry(fs, cluster, FAT12_EOF);
            }
            else {
                fat12_fat_table_set_entry(fs, cluster, FAT12_FREE);
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

uint16_t fat12_acquire_blank_cluster_chain(vfs_t fs, uint32_t n)
{
    assert(fs);
    assert(n > 0);

    // Acquire the first cluster node, and decrement the amount.
    uint16_t first_cluster = fat12_first_available_cluster(fs);
    fat12_fat_table_set_entry(fs, first_cluster, FAT12_EOF);
    --n;

    uint16_t last_cluster = first_cluster;
    while (n--) {
        uint16_t cluster = fat12_first_available_cluster(fs);
        fat12_fat_table_set_entry(fs, last_cluster, cluster);
        fat12_fat_table_set_entry(fs, cluster, FAT12_EOF);
        last_cluster = cluster;
    }

    return first_cluster;
}


#pragma mark - File Creation

void fat12_create_node(vfs_node_t node, const char *name, uint8_t directory)
{
    assert(node);

    fat12_t fat = node->fs->assoc_info;

    // We're going to find the first available cluster for the file.
    uint16_t first_cluster = FAT12_EOF;
    if (directory) {
        uint32_t n = (fat12_root_directory_size(fat->bpb)
                      / fat->bpb->sectors_per_cluster);
        first_cluster = fat12_acquire_blank_cluster_chain(node->fs, n);
    }
    else {
        first_cluster = fat12_acquire_blank_cluster_chain(node->fs, 1);
    }

    // We now need to set that to indicate an end of file.
    fat12_fat_table_set_entry(node->fs, first_cluster, FAT12_EOF);

    // Construct the node accordingly.
    free((void *)node->name);
    node->name = fat12_construct_short_name(name, directory);
    node->is_dirty = 1;
    node->is_directory = directory;
    node->state = vfs_node_used;

    // Update the SFN
    fat12_sfn_t sfn = node->assoc_info;
    memcpy(sfn->name, node->name, 11);
    sfn->first_cluster = first_cluster;
    sfn->attribute |= (node->is_directory ? FAT12_DIRECTORY : 0);

    // Finally convert back from SFN to a regular name
    free((void *)node->name);
    node->name = fat12_construct_standard_name(sfn->name);
}

void fat12_create_directory(vfs_node_t node, const char *name)
{
    assert(node);

    fat12_t fat = node->fs->assoc_info;

    // The first task is to create the actual node required for the directory
    // to be present in the file system.
    fat12_create_node(node, name, 1);
    fat12_sfn_t node_sfn = node->assoc_info;
    vfs_directory_t parent = fat->working_directory;
    fat12_directory_info_t parent_info = parent ? parent->assoc_info : NULL;

    // We can now get the first sector of the directory and prepare to add
    // both the . and .. entries to it. These are required in the file system
    // to allow traversal of the file system.
    fat12_sfn_t current_dir = calloc(1, sizeof(*current_dir));
    fat12_sfn_t parent_dir = calloc(1, sizeof(*parent_dir));

    // The current_directory should point to the new directory represented
    // by node.
    memset(current_dir->name, ' ', 11);
    current_dir->name[0] = '.';
    current_dir->attribute = FAT12_DIRECTORY;
    current_dir->first_cluster = node_sfn->first_cluster;

    // The parent_dir should point to the current working directory
    // of the FAT12 implementation.
    memset(parent_dir->name, ' ', 11);
    parent_dir->name[0] = '.';
    parent_dir->name[1] = '.';
    parent_dir->attribute = FAT12_DIRECTORY;
    parent_dir->first_cluster = parent_info ? parent_info->first_cluster : 0;

    // Now build an entire cluster for this, and write the two entries into it
    uint8_t *data = calloc(fat->bpb->sectors_per_cluster,
                           fat->bpb->bytes_per_sector);
    memcpy(data, current_dir, sizeof(*current_dir));
    memcpy(data + sizeof(*current_dir), parent_dir, sizeof(*parent_dir));

    // Finally write the cluster out to the device
    uint32_t first_sector = fat12_sector_for_cluster(node->fs,
                                                     node_sfn->first_cluster);
    uint32_t sector_count = fat->bpb->sectors_per_cluster;

    device_write_sectors(node->fs->device, first_sector, sector_count, data);

    // Clean up
    free(current_dir);
    free(parent_dir);
    free(data);
}

uint8_t fat12_is_node_available(vfs_node_t node)
{
    return node->state == vfs_node_unused || node->state == vfs_node_available;
}

void fat12_touch(vfs_t fs, const char *name)
{
    assert(fs);
    assert(name);

    fat12_t fat = fs->assoc_info;

    // Find the first available entry in the root directory.
    vfs_node_t node = fat->working_directory->first;
    while (node) {
        // If the node is available then touch this one.
        if (fat12_is_node_available(node)) {
            fat12_create_node(node, name, 0);
            break;
        }
        node = node->next;
    }

    // Flush the directory
    fat12_flush_fat_table(fs);
    fat12_flush_directory(fs);
}

void fat12_make_directory(vfs_t fs, const char *name)
{
    assert(fs);
    assert(name);

    fat12_t fat = fs->assoc_info;

    // Find the first available entry in the root directory.
    vfs_node_t node = fat->working_directory->first;
    while (node) {
        // If the node is available then touch this one.
        if (fat12_is_node_available(node)) {
            fat12_create_directory(node, name);
            break;
        }
        node = node->next;
    }

    // Flush the directory
    fat12_flush_fat_table(fs);
    fat12_flush_directory(fs);
}

