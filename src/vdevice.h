#ifndef VDEVICE
#define VDEVICE

#include <stdio.h>

struct vdev {
    const char *path;
    FILE *handle;
    uint32_t sector_size;
};

typedef struct vdev * vdevice_t;

vdevice_t device_init(const char *restrict path);
vdevice_t device_init_blank(const char *restrict path, uint16_t bs, uint32_t n);
void device_destroy(vdevice_t device);

uint32_t device_total_sectors(vdevice_t device);

uint8_t *device_read_sector(vdevice_t device, uint32_t sector);
uint8_t *device_read_sectors(vdevice_t device, uint32_t sector, uint32_t n);

void device_write_sector(vdevice_t device, uint32_t sector, uint8_t *data);
void device_write_sectors(vdevice_t device, uint32_t sector, uint32_t n,
                          uint8_t *data);

#endif
