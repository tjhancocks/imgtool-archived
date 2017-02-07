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
