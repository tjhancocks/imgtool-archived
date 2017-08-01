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

#ifndef VIRTUAL_DEVICE
#define VIRTUAL_DEVICE

#include <stdio.h>

enum vmedia_type {
    vmedia_floppy = 0x00,
    vmedia_hard_disk = 0x80,
};

struct vdev {
    const char *path;
    FILE *handle;
    uint32_t sector_size;
    enum vmedia_type media;
};

typedef struct vdev * vdevice_t;

vdevice_t device_create(const char *restrict path, enum vmedia_type media);
void device_destroy(vdevice_t device);

uint8_t device_is_inited(vdevice_t dev);
void device_init(vdevice_t dev, uint16_t bps, uint32_t count);

uint32_t device_total_sectors(vdevice_t device);

uint8_t *device_read_sector(vdevice_t device, uint32_t sector);
uint8_t *device_read_sectors(vdevice_t device, uint32_t sector, uint32_t n);

void device_write_sector(vdevice_t device, uint32_t sector, uint8_t *data);
void device_write_sectors(vdevice_t device, uint32_t sector, uint32_t n,
                          uint8_t *data);

#endif
