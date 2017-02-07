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

#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include "vdevice.h"


vdevice_t device_init(const char *restrict path)
{
    FILE *f = fopen(path, "rb+");
    if (!f) {
        fprintf(stderr, "Failed to open device file: %s\n", path);
        return NULL;
    }

    vdevice_t dev = calloc(1, sizeof(*dev));
    dev->handle = f;
    dev->path = calloc(strlen(path)+1, sizeof(*dev->path));
    dev->sector_size = 512;
    return dev;
}

vdevice_t device_init_blank(const char *restrict path, uint16_t bs, uint32_t n)
{
    FILE *f = fopen(path, "wb+");
    if (!f) {
        fprintf(stderr, "Failed to create device file: %s\n", path);
        return NULL;
    }

    vdevice_t dev = calloc(1, sizeof(*dev));
    dev->handle = f;
    dev->path = calloc(strlen(path)+1, sizeof(*dev->path));
    dev->sector_size = bs;

    // write zeros out to the file. create a blank sector.
    uint8_t *sector = calloc(dev->sector_size, sizeof(*sector));
    for (uint32_t i = 0; i < n; ++i) {
        fwrite(sector, sizeof(*sector), dev->sector_size, dev->handle);
    }
    fflush(dev->handle);
    free(sector);

    return dev;
}

void device_destroy(vdevice_t device)
{
    if (device) {
        fclose(device->handle);
        free((void *)device->path);
    }
    free(device);
}


uint32_t device_total_sectors(vdevice_t device)
{
    assert(device);
    fseek(device->handle, 0L, SEEK_END);
    uint32_t n = (uint32_t)ftell(device->handle);
    fseek(device->handle, 0L, SEEK_SET);
    return n / device->sector_size;
}


uint8_t *device_read_sector(vdevice_t device, uint32_t sector)
{
    assert(device);
    assert(sector < device_total_sectors(device));

    uint8_t *data = calloc(device->sector_size, sizeof(*data));
    fseek(device->handle, sector * device->sector_size, SEEK_SET);
    fread(data, sizeof(*data), device->sector_size, device->handle);
    return data;
}

uint8_t *device_read_sectors(vdevice_t device, uint32_t sector, uint32_t n)
{
    assert(device);
    assert(sector < device_total_sectors(device));
    assert(sector + n < device_total_sectors(device));

    uint8_t *data = calloc(n * device->sector_size, sizeof(*data));
    fseek(device->handle, sector * device->sector_size, SEEK_SET);
    fread(data, sizeof(*data), n * device->sector_size, device->handle);
    return data;
}

void device_write_sector(vdevice_t device, uint32_t sector, uint8_t *data)
{
    assert(device);
    assert(sector < device_total_sectors(device));

    fseek(device->handle, sector * device->sector_size, SEEK_SET);
    fwrite(data, sizeof(*data), device->sector_size, device->handle);
    fflush(device->handle);
}

void device_write_sectors(vdevice_t device, uint32_t sector, uint32_t n,
                          uint8_t *data)
{
    assert(device);
    assert(sector < device_total_sectors(device));
    assert(sector + n < device_total_sectors(device));

    fseek(device->handle, sector * device->sector_size, SEEK_SET);
    fwrite(data, sizeof(*data), device->sector_size * n, device->handle);
    fflush(device->handle);
}
